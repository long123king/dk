#include "model.h"
#include "MemoryAnalyzer.h"
#include "CmdExt.h"
#include "CmdList.h"

#include <sstream>
#include <iomanip>


size_t CMemoryAnalyzer::is_in_cur_stack(size_t addr)
{
    auto cur_callstacks = DK_GET_CURSTACK();

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
    if (addr >= m_addr && addr < m_addr + m_len)
        return true;

    return false;
}

string CMemoryAnalyzer::get_symbol(size_t addr)
{
    auto sym = EXT_F_Addr2Sym(addr);
    if (get<0>(sym).empty())
        return "";

    stringstream ss;

    ss << get<0>(sym) << "+0x" << hex << get<1>(sym);

    return ss.str();
}

tuple<size_t, size_t> CMemoryAnalyzer::is_heap(size_t addr)
{
    for (auto& entry : m_heap)
    {
        if (addr >= entry.res_id && addr < entry.res_id + entry.size)
        {
            return make_tuple(entry.res_id, entry.size);
        }
    }

    return make_tuple(0, 0);;
}

void CMemoryAnalyzer::analyze()
{
    map<size_t, string> notes;
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
            m_ptr2heaps[i * 8] = make_tuple(qw, get<0>(heap_entry), get<1>(heap_entry));
        }
    }
}

map<size_t, size_t> CMemoryAnalyzer::get_ptr2local()
{
    return m_ptr2locals;
}

map<size_t, size_t> CMemoryAnalyzer::get_ptr2stack()
{
    return m_ptr2stacks;
}

map<size_t, tuple<string, size_t>> CMemoryAnalyzer::get_ptr2sym()
{
    return m_ptr2syms;
}

map<size_t, tuple<size_t, size_t, size_t>> CMemoryAnalyzer::get_ptr2heap()
{
    return m_ptr2heaps;
}
