#include "model.h"
#include "MemoryAnalyzer.h"
#include "CmdExt.h"
#include "CmdList.h"

#include <sstream>
#include <iomanip>


size_t CMemoryAnalyzer::is_in_cur_stack(size_t addr, size_t id)
{
    static std::map<size_t, std::vector<std::array<uint64_t, 5>>> s_cache;

    if (s_cache.find(id) == s_cache.end())
        s_cache[id] = DK_GET_CURSTACK();

    auto cur_callstacks = s_cache[id];

    for (auto& callstack : cur_callstacks)
    {
        if (addr <= callstack[3] && addr >= callstack[4])
        {
            return callstack[0];
        }
    }

    return -1;
}

bool CMemoryAnalyzer::is_local(size_t addr)
{
    if (addr >= m_local_start && addr < m_local_end)
        return true;

    return false;
}

std::string CMemoryAnalyzer::get_symbol(size_t addr)
{
    auto sym = EXT_F_Addr2Sym(addr);
    if (get<0>(sym).empty())
        return "";

    std::stringstream ss;

    ss << get<0>(sym) << "+0x" << std::hex << get<1>(sym);

    return ss.str();
}

std::tuple<size_t, size_t> CMemoryAnalyzer::is_heap(size_t addr)
{
    for (auto& entry : m_heap)
    {
        if (addr >= entry.res_id && addr < entry.res_id + entry.size)
        {
            return std::make_tuple(entry.res_id, entry.size);
        }
    }

    return std::make_tuple(0, 0);;
}

void CMemoryAnalyzer::carve_strings()
{
    size_t handle_len = m_len;
    if (m_len > 0x1000)
        handle_len = 0x1000;

    std::string page(handle_len, '0');

    size_t bytes_read = 0;
    HRESULT result = EXT_D_IDebugDataSpaces->ReadVirtual(m_addr, (uint8_t*)page.data(), handle_len, (PULONG)&bytes_read);
    if (result != 0 || bytes_read != handle_len)
    {
        EXT_F_ERR("Fail to read memory in [ 0x%0I64x - 0x%0I64x ], HRESULT: 0x%08x\n", m_addr, m_addr + handle_len, result);

        return;
    }

    std::string str;
    for (size_t i = 0; i < handle_len; i++)
    {
        if ((page[i] & 0x80) == 0 && isprint(page[i]))
        {
            str.push_back(page[i]);
            continue;
        }
        else
        {
            if (str.length() >= 5)
            {
                m_ptr2astrs[m_addr + i - str.length()] = str;
            }
            str.clear();
        }
    }
    if (str.length() >= 5)
    {
        m_ptr2astrs[m_addr + handle_len - str.length()] = str;
    }
}

void CMemoryAnalyzer::carve_ustrings()
{
    size_t handle_len = m_len;
    if (m_len > 0x1000)
        handle_len = 0x1000;

    std::string page(handle_len, '0');

    size_t bytes_read = 0;
    HRESULT result = EXT_D_IDebugDataSpaces->ReadVirtual(m_addr, (uint8_t*)page.data(), handle_len, (PULONG)&bytes_read);
    if (result != 0 || bytes_read != handle_len)
    {
        EXT_F_ERR("Fail to read memory in [ 0x%0I64x - 0x%0I64x ], HRESULT: 0x%08x\n", m_addr, m_addr + handle_len, result);

        return;
    }

    std::string str;
    for (size_t i = 0; i < handle_len; i += 2)
    {
        if ((page[i] & 0x80) == 0 && isprint(page[i]))
        {
            str.push_back(page[i]);
            continue;
        }
        else if ((page[i] == 0 && page[i + 1] == 0) || 
            i == handle_len)
        {
            if (str.length() >= 5)
            {
                std::stringstream ss;

                m_ptr2ustrs[m_addr + i - str.length() * 2] = str;
                str.clear();
            }
            str.clear();
        }
        else
        {
            if (str.length() >= 5)
            {
                std::stringstream ss;

                m_ptr2ustrs[m_addr + i - str.length() * 2] = str;
                str.clear();
            }
            str.clear();
        }

    }
    if (str.length() >= 5)
    {
        m_ptr2ustrs[m_addr + handle_len - str.length() * 2] = str;
    }
}
void CMemoryAnalyzer::analyze()
{
    std::map<size_t, std::string> notes;
    uint64_t* qwords = (uint64_t*)m_data.data();

    for (auto i = 0; i < m_len / 8; i++)
    {
        uint64_t qw = *(qwords + i);
        if (is_local(qw))
        {
            m_ptr2locals[i * 8] = qw - m_addr;
        }

        auto frame_num = is_in_cur_stack(qw);
        if (frame_num != -1)
        {
            m_ptr2stacks[i * 8] = frame_num;
        }

        auto sym = get_symbol(qw);
        if (!sym.empty())
        {
            m_ptr2syms[i * 8] = make_tuple(sym, qw);
        }

        auto heap_entry = is_heap(qw);
        if (get<1>(heap_entry) != 0)
        {
            m_ptr2heaps[i * 8] = std::make_tuple(qw, get<0>(heap_entry), get<1>(heap_entry));
        }
    }

    carve_strings();
    carve_ustrings();
    detect_calls();
}

void CMemoryAnalyzer::detect_calls()
{
    if (m_data.size() < 5) return;

    const uint8_t* bytes = (const uint8_t*)m_data.data();
    const size_t PAGE_SIZE = 0x1000;

    for (size_t i = 0; i < PAGE_SIZE && i < m_data.size(); i++)
    {
        int instrLen = 0;
        int32_t rel32 = 0;
        std::string kind;

        if ((bytes[i] == 0xE8 || bytes[i] == 0xE9) && i + 4 < PAGE_SIZE)
        {
            uint32_t u = bytes[i + 1] | (bytes[i + 2] << 8) | (bytes[i + 3] << 16) | (bytes[i + 4] << 24);
            rel32 = (int32_t)u;
            instrLen = 5;
            kind = (bytes[i] == 0xE8) ? "call" : "jmp";
        }
        else if (bytes[i] == 0xEB && i + 1 < PAGE_SIZE)
        {
            rel32 = (int8_t)bytes[i + 1];
            instrLen = 2;
            kind = "jmp8";
        }

        if (instrLen == 0) continue;

        size_t target = m_addr + i + instrLen + rel32;
        bool inPage = (target >= m_addr && target < m_addr + PAGE_SIZE);

        std::string sym = get_symbol(target);
        if (!inPage && sym.empty())
            continue; // only show meaningful targets

        m_ptr2calls[i] = std::make_tuple(kind, target, sym, instrLen);
    }
}

std::map<size_t, std::tuple<std::string, size_t, std::string, int>> CMemoryAnalyzer::get_ptr2call()
{
    return m_ptr2calls;
}

std::map<size_t, size_t> CMemoryAnalyzer::get_ptr2local()
{
    return m_ptr2locals;
}

std::map<size_t, size_t> CMemoryAnalyzer::get_ptr2stack()
{
    return m_ptr2stacks;
}

std::map<size_t, std::tuple<std::string, size_t>> CMemoryAnalyzer::get_ptr2sym()
{
    return m_ptr2syms;
}

std::map<size_t, std::tuple<size_t, size_t, size_t>> CMemoryAnalyzer::get_ptr2heap()
{
    return m_ptr2heaps;
}

std::map<size_t, std::string> CMemoryAnalyzer::get_ptr2astr()
{
    return m_ptr2astrs;
}

std::map<size_t, std::string> CMemoryAnalyzer::get_ptr2ustr()
{
    return m_ptr2ustrs;
}