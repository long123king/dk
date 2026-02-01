#include "model.h"
#include "usermode.h"
#include "CmdExt.h"
#include "CmdList.h"
#include "memory.h"
#include "hexsvg.h"

#include <set>
#include <regex>
#include <iomanip>
#include <fstream>

#define ZERO_REPEAT_THREADHOLD 4


DEFINE_CMD(usearch_astr)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 2)
    {
        EXT_F_OUT("Usage: !dk usearch_astr <pattern> [ref_by_tree_level]\nUser Mode Only\n");
        return;
    }

    size_t level = EXT_F_IntArg(args, 2, 0);

    usermode_search_astr(args[1], level);
}

DEFINE_CMD(usearch_ustr)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 2)
    {
        EXT_F_OUT("Usage: !dk usearch_ustr <pattern> [ref_by_tree_level]\nUser Mode Only\n");
        return;
    }

    size_t level = EXT_F_IntArg(args, 2, 0);

    usermode_search_ustr(args[1], level);
}

DEFINE_CMD(usearch_bytes)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 2)
    {
        EXT_F_OUT("Usage: !dk usearch_bytes <pattern> [ref_by_tree_level]\nUser Mode Only\n");
        return;
    }

    size_t level = EXT_F_IntArg(args, 2, 0);

    usermode_search_bytes(args[1], level);
}

DEFINE_CMD(usearch_addr)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 2)
    {
        EXT_F_OUT("Usage: !dk usearch_addr <pattern> [ref_by_tree_level]\nUser Mode Only\n");
        return;
    }

    size_t addr = EXT_F_IntArg(args, 1, 0);
    size_t level = EXT_F_IntArg(args, 2, 0);

    usermode_search_addr(addr, level);
}

DEFINE_CMD(uaddr_analyze)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 3)
    {
        EXT_F_OUT("Usage: !dk uaddr_analyze <start> <len>\nUser Mode Only\n");
        return;
    }

    size_t start = EXT_F_IntArg(args, 1, 0);
    size_t len = EXT_F_IntArg(args, 2, 0);

    size_t offset = 0;
    if (args.size() == 4)
        offset = EXT_F_IntArg(args, 3, 0);

    usermode_addr_analyze(start, len, offset);
}

DEFINE_CMD(uaddr_analyze_svg)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 4)
    {
        EXT_F_OUT("Usage: !dk uaddr_analyze_svg <start> <len> <svg_name>\nUser Mode Only\n");
        return;
    }

    size_t start = EXT_F_IntArg(args, 1, 0);
    size_t len = EXT_F_IntArg(args, 2, 0);
    std::string svg_name = args[3];

    size_t offset = 0;
    if (args.size() == 5)
        offset = EXT_F_IntArg(args, 4, 0);

    usermode_addr_analyze_svg(start, len, svg_name, offset);
}

DEFINE_CMD(uaddr_analyze2)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 3)
    {
        EXT_F_OUT("Usage: !dk uaddr_analyze <start> <len>\nUser Mode Only\n");
        return;
    }

    size_t start = EXT_F_IntArg(args, 1, 0);
    size_t len = EXT_F_IntArg(args, 2, 0);

    size_t offset = 0;
    if (args.size() == 4)
        offset = EXT_F_IntArg(args, 3, 0);

    usermode_addr_analyze2(start, len, offset);
}

DEFINE_CMD(uaddr_ref_disp)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 6)
    {
        EXT_F_OUT("Usage: !dk uaddr_ref_disp <start> <len> <ref_start> <ref_len> <ref_disp>\nUser Mode Only\n");
        return;
    }

    size_t start = EXT_F_IntArg(args, 1, 0);
    size_t len = EXT_F_IntArg(args, 2, 0);
    size_t ref_start = EXT_F_IntArg(args, 3, 0);
    size_t ref_len = EXT_F_IntArg(args, 4, 0);
    size_t ref_disp = EXT_F_IntArg(args, 5, 0);

    usermode_addr_ref_disp(start, len, ref_start, ref_len, ref_disp);
}

DEFINE_CMD(uaddr_ref_by)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 2)
    {
        EXT_F_OUT("Usage: !dk uaddr_ref_by\nUser Mode Only\n");
        return;
    }

    size_t start = EXT_F_IntArg(args, 1, 0);

    usermode_addr_ref_by(start);
}

DEFINE_CMD(uaddr_ref_tree)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 3)
    {
        EXT_F_OUT("Usage: !dk uaddr_ref_tree <addr> <level>\nUser Mode Only\n");
        return;
    }

    size_t start = EXT_F_IntArg(args, 1, 0);
    size_t level = EXT_F_IntArg(args, 2, 10);

    usermode_addr_ref_tree(start, level);
}

DEFINE_CMD(ustacks)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 1)
    {
        EXT_F_OUT("Usage: !dk ustacks\nUser Mode Only\n");
        return;
    }

    usermode_stacks();
}

DEFINE_CMD(uregs)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 1)
    {
        EXT_F_OUT("Usage: !dk uregs\nUser Mode Only\n");
        return;
    }

    usermode_regs();
}

void usermode_search_astr(std::string pattern, size_t level)
{
    try
    {
        ULONG64 start = 0;
        ULONG64 end = 0x7fffffff0000;

        ULONG64 length = 0x8000000;

        EXT_F_OUT("Searching Virtual Memory in range(0x%0I64x, 0x%0I64x, 0x%0I64x) for ASCII String %s\n\n", start, end, length, pattern.c_str());
        ULONG64 match_offset = 0;

        while (start < end)
        {
            auto hr = EXT_D_IDebugDataSpaces4->SearchVirtual2(start, length, DEBUG_VSEARCH_WRITABLE_ONLY, (PVOID)pattern.c_str(), pattern.size(), 1, &match_offset);
            if (S_OK == hr)
            {
                EXT_F_OUT("\nFound pattern %s at 0x%0I64x\n", pattern.c_str(), match_offset);

                std::stringstream dml_ss;

                dml_ss << DML_CMD << "!dk uaddr_ref_by " << std::hex << std::showbase << match_offset
                    << DML_TEXT << "ref_by"
                    << DML_END
                    << std::endl;

                EXT_F_STR_DML(dml_ss);

                CUsermodeMemory::GetInstance()->analyze_mem((match_offset - 0x40) & 0xFFFFFFFFFFF0, 0x80, (match_offset - 0x40) & 0xFFFFFFFFFFF0);

                if (level != 0)
                {
                    CUsermodeMemory::GetInstance()->set_ref_by_tree_levels(level);

                    CUsermodeMemory::GetInstance()->analyze_ref_tree(match_offset);
                }

                start = match_offset + pattern.size();
            }
            else if (0x9000001A == hr)
            {
                //EXT_F_OUT("Not Found\n");

                start += length;
            }
            else
            {
                EXT_F_OUT("Error: 0x%08x\n", hr);
                break;
            }
        }
    }
    FC;
}

void usermode_search_ustr(std::string pattern, size_t level)
{
    try
    {
        ULONG64 start = 0;
        ULONG64 end = 0x7fffffff0000;

        ULONG64 length = 0x8000000;

        EXT_F_OUT("Searching Virtual Memory in range(0x%0I64x, 0x%0I64x, 0x%0I64x) for Unicode String %s\n\n", start, end, length, pattern.c_str());
        ULONG64 match_offset = 0;

        std::wstring wstr_pattern(pattern.begin(), pattern.end());

        while (start < end)
        {
            auto hr = EXT_D_IDebugDataSpaces4->SearchVirtual2(start, length, DEBUG_VSEARCH_WRITABLE_ONLY, (PVOID)wstr_pattern.c_str(), wstr_pattern.size()*2, 1, &match_offset);
            if (S_OK == hr)
            {
                EXT_F_OUT("\nFound pattern %s at 0x%0I64x\n", pattern.c_str(), match_offset);

                std::stringstream dml_ss;

                dml_ss << DML_CMD << "!dk uaddr_ref_by " << std::hex << std::showbase << match_offset
                    << DML_TEXT << "ref_by"
                    << DML_END
                    << std::endl;

                EXT_F_STR_DML(dml_ss);

                CUsermodeMemory::GetInstance()->analyze_mem((match_offset - 0x40)&0xFFFFFFFFFFF0, 0x80, (match_offset - 0x40) & 0xFFFFFFFFFFF0);

                if (level != 0)
                {
                    CUsermodeMemory::GetInstance()->set_ref_by_tree_levels(level);

                    CUsermodeMemory::GetInstance()->analyze_ref_tree(match_offset);
                }

                start = match_offset + wstr_pattern.size()*2;
            }
            else if (0x9000001A == hr)
            {
                //EXT_F_OUT("Not Found\n");

                start += length;
            }
            else
            {
                EXT_F_OUT("Error: 0x%08x\n", hr);
                break;
            }
        }
    }
    FC;
}

void usermode_search_bytes(std::string bytes, size_t level)
{
    try
    {
        ULONG64 start = 0;
        ULONG64 end = 0x7fffffff0000;

        ULONG64 length = 0x8000000;

        std::string pattern;

        if (bytes.size() % 2 != 0)
        {
            EXT_F_OUT("Error: Wrong Bytes\n");
            return;
        }
        else
        {
            for (size_t i = 0; i < bytes.size() / 2; i++)
            {
                char byte = (char)(std::stoll(bytes.substr(i * 2, 2), nullptr, 0x10));
                pattern.push_back(byte);
            }
        }

        EXT_F_OUT("Searching Virtual Memory in range(0x%0I64x, 0x%0I64x, 0x%0I64x) for Bytes %s\n\n", start, end, length, pattern.c_str());

        ULONG64 match_offset = 0;

        while (start < end)
        {
            auto hr = EXT_D_IDebugDataSpaces4->SearchVirtual2(start, length, DEBUG_VSEARCH_WRITABLE_ONLY, (PVOID)pattern.c_str(), pattern.size(), 1, &match_offset);
            if (S_OK == hr)
            {
                EXT_F_OUT("\nFound pattern %s at 0x%0I64x\n", bytes.c_str(), match_offset);

                std::stringstream dml_ss;

                dml_ss << DML_CMD << "!dk uaddr_ref_by " << std::hex << std::showbase << match_offset
                    << DML_TEXT << "ref_by"
                    << DML_END
                    << std::endl;

                EXT_F_STR_DML(dml_ss);

                CUsermodeMemory::GetInstance()->analyze_mem((match_offset - 0x40) & 0xFFFFFFFFFFF0, 0x80, (match_offset - 0x40) & 0xFFFFFFFFFFF0);

                if (level != 0)
                {
                    CUsermodeMemory::GetInstance()->set_ref_by_tree_levels(level);

                    CUsermodeMemory::GetInstance()->analyze_ref_tree(match_offset);
                }

                start = match_offset + pattern.size();
            }
            else if (0x9000001A == hr)
            {
                //EXT_F_OUT("Not Found\n");

                start += length;
            }
            else
            {
                EXT_F_OUT("Error: 0x%08x\n", hr);
                break;
            }
        }
    }
    FC;
}

void usermode_search_addr(size_t addr, size_t level)
{
    try
    {
        ULONG64 start = 0;
        ULONG64 end = 0x7fffffff0000;

        ULONG64 length = 0x8000000;

        std::string pattern;

        EXT_F_OUT("Searching Virtual Memory in range(0x%0I64x, 0x%0I64x, 0x%0I64x) for Address 0x%0I64x\n\n", start, end, length, addr);

        ULONG64 match_offset = 0;

        while (start < end)
        {
            auto hr = EXT_D_IDebugDataSpaces4->SearchVirtual2(start, length, DEBUG_VSEARCH_WRITABLE_ONLY, &addr, 8, 8, &match_offset);
            if (S_OK == hr)
            {
                EXT_F_OUT("\nFound pattern 0x%0I64x at 0x%0I64x\n", addr, match_offset);

                std::stringstream dml_ss;

                dml_ss << DML_CMD << "!dk uaddr_ref_by " << std::hex << std::showbase << match_offset
                    << DML_TEXT << "ref_by"
                    << DML_END
                    << std::endl;

                EXT_F_STR_DML(dml_ss);

                CUsermodeMemory::GetInstance()->analyze_mem((match_offset - 0x40) & 0xFFFFFFFFFFF0, 0x80, (match_offset - 0x40) & 0xFFFFFFFFFFF0);

                if (level != 0)
                {
                    CUsermodeMemory::GetInstance()->set_ref_by_tree_levels(level);

                    CUsermodeMemory::GetInstance()->analyze_ref_tree(match_offset);
                }

                start = match_offset + 8;
            }
            else if (0x9000001A == hr)
            {
                //EXT_F_OUT("Not Found\n");

                start += length;
            }
            else
            {
                EXT_F_OUT("Error: 0x%08x\n", hr);
                break;
            }
        }
    }
    FC;
}

void usermode_addr_ref_disp(size_t start, size_t len, size_t ref_start, size_t ref_len, size_t ref_disp)
{
    try
    {
        EXT_F_OUT("Searching Usermode Address References in range(0x%0I64x, 0x%0I64x) for References to (0x%0I64x, 0x%0I64x), disp to 0x%0I64x\n\n", 
            start, start + len,
            ref_start, ref_start + ref_len, ref_disp);

        size_t end = start + len;

        std::set<size_t> qws;

        while (start < end)
        {
            size_t qw = EXT_F_READ<size_t>(start);

            if (qw >= ref_start && qw < ref_start + ref_len)
                qws.insert(ref_disp + (qw - ref_start));

            start += 8;
        }

        for (auto qw : qws)
        {
            EXT_F_OUT("0x%0I64x  :  0x%0I64x\n", qw - ref_disp + ref_start, qw);
        }
    }
    FC;
}

void usermode_addr_ref_by(size_t start)
{
    try
    {
        EXT_F_OUT("Usermode Address Referenced By Analysis for 0x%0I64x\n\n",
            start);

        CUsermodeMemory::GetInstance()->analyze_ref_by(start);
    }
    FC;
}

void usermode_addr_ref_tree(size_t start, size_t level)
{
    try
    {
        EXT_F_OUT("Usermode Address Referenced Tree Analysis for 0x%0I64x\n\n",
            start);

        CUsermodeMemory::GetInstance()->set_ref_by_tree_levels(level);

        CUsermodeMemory::GetInstance()->analyze_ref_tree(start);
    }
    FC;
}

void usermode_addr_analyze(size_t start, size_t len, size_t offset)
{
    try
    {
        EXT_F_OUT("Usermode Address Analyze in range(0x%0I64x, 0x%0I64x)\n\n",
            start, start + len);

        CUsermodeMemory::GetInstance()->analyze_mem(start, len, offset);
    }
    FC;
}

void usermode_addr_analyze_svg(size_t start, size_t len, std::string svg_filename, size_t offset)
{
    try
    {
        EXT_F_OUT("Usermode Address Analyze in range(0x%0I64x, 0x%0I64x)\n\n",
            start, start + len);

        CUsermodeMemory::GetInstance()->analyze_mem_svg(start, len, offset, svg_filename);
    }
    FC;
}

void usermode_addr_analyze2(size_t start, size_t len, size_t offset)
{
    try
    {
        EXT_F_OUT("Usermode Address Analyze with references in range(0x%0I64x, 0x%0I64x)\n\n",
            start, start + len);

        CUsermodeMemory::GetInstance()->analyze_mem(start, len, offset, true);
    }
    FC;
}

void usermode_stacks()
{
    try
    {
        CUsermodeMemory::GetInstance()->dump_all_stacks();
    }
    FC;
}

void usermode_regs()
{
    static std::vector<const char*> regs_name{
        "rip", "rsp",
            "rax", "rbx", "rcx", "rdx",
            "rbp", "rsi", "rdi",
            "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };

    try
    {
        CUsermodeMemory::GetInstance()->dummy();

        for (auto& reg_entry : regs_name)
        {
            std::stringstream ss;
            size_t reg_value = EXT_F_REG_OF(reg_entry);
            ss << "                " << reg_entry << ": \t" << std::hex << std::showbase << std::setw(18) << reg_value << " ";
            EXT_F_OUT(ss.str().c_str());
            CUsermodeMemory::GetInstance()->analyze_qword(reg_value);
        }
    }
    FC;
}

CUsermodeStack::CUsermodeStack(size_t start, size_t end, size_t size, size_t t_index, size_t tid, size_t pid, std::string usage)
    :m_start(start)
    , m_end(end)
    , m_size(size)
    , m_t_index(t_index)
    , m_tid(tid)
    , m_pid(pid)
    , m_usage_str(usage)
{
    //stringstream ss;
    //ss << "Stack: " 
    //    << hex << showbase
    //    << m_start << " "
    //    << m_end << " "
    //    << m_size << " "
    //    << m_t_index << " "
    //    << m_tid << " "
    //    << m_pid << endl;

    //EXT_F_STR_OUT(ss);
}

CUsermodeStack::~CUsermodeStack()
{
}

void CUsermodeStack::Analyze()
{
}

std::string CUsermodeStack::desc(size_t addr)
{
    std::stringstream ss;

    if (in_range(addr))
    {
        ss  << std::hex << std::showbase << m_start << " - " << m_end
            << " thread : " 
            << std::hex << std::showbase << m_t_index << " " 
            << m_pid << " : " << m_tid;
    }

    return ss.str();
}


std::tuple<size_t, size_t> CUsermodeStack::dump()
{
    EXT_F_OUT("Stack: 0x%x %x:%x\n", m_t_index, m_pid, m_tid);

    return std::make_tuple(m_start, m_size);
}

CUsermodeModule::CUsermodeModule(size_t start, size_t end, size_t size, std::string name, std::string path, std::string usage)
    :m_start(start)
    , m_end(end)
    , m_size(size)
    , m_name(name)
    , m_path(path)
    , m_usage_str(usage)
{
    //stringstream ss;
    //ss  << "Module: "
    //    << hex << showbase
    //    << m_start << " "
    //    << m_end << " "
    //    << m_size << " "
    //    << m_name << " "
    //    << m_path << endl;

    //EXT_F_STR_OUT(ss);
}

CUsermodeModule::~CUsermodeModule()
{
}

void CUsermodeModule::Analyze()
{
}

std::string CUsermodeModule::code_desc(size_t addr)
{
    std::stringstream ss;

    static std::map<size_t, std::tuple<std::string, uint64_t>> s_cache;

    if (in_range(addr))
    {
        if (s_cache.find(addr) == s_cache.end())
        {
            auto sym_info = EXT_F_Addr2Sym(addr);
            s_cache[addr] = sym_info;
        }          

        ss << get<0>(s_cache[addr]) << "+" << std::hex << std::showbase << get<1>(s_cache[addr]);
    }

    return ss.str();
}

std::string CUsermodeModule::data_desc(size_t addr)
{
    std::stringstream ss;

    static std::map<size_t, std::tuple<std::string, uint64_t>> s_cache;

    if (in_range(addr))
    {
        if (s_cache.find(addr) == s_cache.end())
        {
            auto sym_info = EXT_F_Addr2Sym(addr);

            if (get<0>(sym_info).empty())
            {
                std::stringstream data_info_ss;

                data_info_ss << "Module: " << m_name << " offset: "
                    << std::hex << std::showbase << addr - m_start << " ";

                if (is_readonly())
                {
                    data_info_ss << "ReadOnly ";
                }
                else if (is_global())
                {
                    data_info_ss << "Global ";
                }

                std::string attr = m_usage_str;
                auto pos = attr.find("Image");
                if (pos != std::string::npos)
                    attr = attr.substr(0, pos - 1);

                data_info_ss << attr;

                get<0>(sym_info) = data_info_ss.str();
            }

            s_cache[addr] = sym_info;
        }

        ss << get<0>(s_cache[addr]);
        if (get<1>(s_cache[addr]) != 0)
         ss << "+" << std::hex << std::showbase << get<1>(s_cache[addr]);
    }

    return ss.str();
}


CUsermodeHeap::CUsermodeHeap(size_t start, size_t end, size_t size, size_t id, size_t handle, std::string type, std::string usage)
    :m_start(start)
    , m_end(end)
    , m_size(size)
    , m_id(id)
    , m_handle(handle)
    , m_type(type)
    , m_usage_str(usage)
{
    //stringstream ss;
    //ss << "Heap: " 
    //    << hex << showbase
    //    << m_start << " "
    //    << m_end << " "
    //    << m_size << " "
    //    << m_id << " "                  
    //    << m_handle << " "
    //    << m_type << endl;

    //EXT_F_STR_OUT(ss);
}

CUsermodeHeap::~CUsermodeHeap()
{
}

void CUsermodeHeap::Analyze()
{
}

std::string CUsermodeHeap::desc(size_t addr)
{
    std::stringstream ss;

    static std::map<size_t, std::tuple<size_t, size_t, size_t, std::string>> s_cache;

    static std::regex s_heap_info_regex(R"(([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+(\-|[0-9a-f]+)\s+(\-|[0-9a-f]+)\s+(\-|[0-9a-f]+)\s+(.+))");

    if (in_range(addr))
    {
        if (s_cache.find(addr) == s_cache.end())
        {
            std::stringstream ss_cmd;
            ss_cmd << "!heap -x " << std::hex << std::showbase << addr;

            auto results = DK_X_CMD(ss_cmd.str());

            bool b_multiline_info = false;
            size_t heap = 0;
            size_t user_ptr = 0;
            size_t size = 0;

            for (auto& result : results)
            {
                if (!b_multiline_info)
                {
                    std::smatch sm;
                    regex_match(result, sm, s_heap_info_regex);

                    if (sm.size() == 9)
                    {
                        size_t entry = std::stoll(sm[1], nullptr, 16);
                        size_t user_ptr = std::stoll(sm[2], nullptr, 16);
                        size_t heap = std::stoll(sm[3], nullptr, 16);
                        size_t segment = std::stoll(sm[4], nullptr, 16);

                        std::string size_str = sm[5];
                        std::string prev_size_str = sm[6];
                        std::string unused_str = sm[7];
                        std::string flags_str = sm[8];

                        size_t size = 0;
                        size_t prev_size = 0;
                        size_t unused = 0;

                        if (size_str.find('-') == std::string::npos)
                            size = std::stoll(size_str, nullptr, 16);

                        if (prev_size_str.find('-') == std::string::npos)
                            prev_size = std::stoll(prev_size_str, nullptr, 16);

                        if (unused_str.find('-') == std::string::npos)
                            unused = std::stoll(unused_str, nullptr, 16);

                        s_cache[addr] = std::make_tuple(user_ptr, size, heap, flags_str);
                        break;
                    }
                    else if (result.find("Below is a detailed information about this address.") == 0)
                    {
                        b_multiline_info = true;
                    }
                }
                else if (result.find("Block Address            : ") == 0)
                {
                    user_ptr = std::stoll(result.substr(0x1B), nullptr, 16);
                }
                else if (result.find("Block Size               : ") == 0)
                {
                    size = std::stoll(result.substr(0x1B), nullptr, 10);
                }
                else if (result.find("Heap Address             : ") == 0)
                {
                    heap = std::stoll(result.substr(0x1B), nullptr, 16);
                }
            }

            s_cache[addr] = std::make_tuple(user_ptr, size, heap, "");
        }

        if (s_cache.find(addr) != s_cache.end())
        {
            auto alloc = CUsermodeMemory::GetInstance()->get_allocation(get<0>(s_cache[addr]), get<0>(s_cache[addr]) + get<1>(s_cache[addr]));
                                                  
            if (!alloc)
            {
            }
            else if (alloc->desc().empty())
            {
                ss << DML_CMD << "!dk uaddr_analyze " << std::hex << std::showbase << get<0>(s_cache[addr]) << " " << get<1>(s_cache[addr]) /*<< " " << get<0>(s_cache[addr])*/
                    << DML_TEXT << " [ " << std::hex << std::showbase << get<0>(s_cache[addr]) << " - " << (get<0>(s_cache[addr]) + get<1>(s_cache[addr]))
                    << " ] len: " << get<1>(s_cache[addr]) << " handle: "
                    << get<2>(s_cache[addr]) << " flag: " << get<3>(s_cache[addr])
                    << DML_END;

                alloc->set_desc(ss.str());
            }
            else
            {
                return alloc->desc();
            }
        }
    }

    return ss.str();
}

std::string CUsermodeHeap::ref_by_desc(size_t addr)
{
    std::stringstream ss;
    
    ss << std::hex << std::showbase << " referenced by: " << std::endl;

    for (auto ref : get<1>(ref_by(addr)))
    {
        ss << DML_CMD << "!dk uaddr_ref_by " << std::hex << std::showbase << ref
            << DML_TEXT << " * " << std::hex << std::showbase << ref
            << DML_END
            << std::endl;
    }
    
    return ss.str();
}

std::tuple<std::vector<std::shared_ptr<CUsermodeHeapAllocation>>, std::set<size_t>> CUsermodeHeap::ref_by(size_t addr)
{
    static std::regex s_search_vm_regex(R"(Search VM for address range ([0-9a-f]+) - ([0-9a-f]+) : (.*))");

    static std::map<size_t, std::set<size_t>> s_cache;
    static std::map<size_t, std::vector<std::shared_ptr<CUsermodeHeapAllocation>>> s_cache_allocations;

    std::vector<std::shared_ptr<CUsermodeHeapAllocation>> allocations;
    try
    {
        if (s_cache.find(addr) == s_cache.end())
        {
            std::stringstream ss_cmd;

            ss_cmd << "!heap -x -v " << std::hex << std::showbase << addr;

            std::set<size_t> refs_set;

            auto results = DK_X_CMD(ss_cmd.str());
            for (auto& result : results)
            {
                std::smatch sm;
                regex_match(result, sm, s_search_vm_regex);

                if (sm.size() == 4)
                {
                    size_t start = std::stoll(sm[1], nullptr, 16);
                    size_t end = std::stoll(sm[2], nullptr, 16);

                    auto alloc = CUsermodeMemory::GetInstance()->get_allocation(start, end);

                    std::istringstream refs_ss(sm[3]);
                    std::string ref;

                    while (std::getline(refs_ss, ref, ',')) {
                        bool contains_hex = false;
                        for (auto& ch : ref)
                        {
                            if (EXT_F_IS_IN_RANGE<char>(ch, '0', '9')
                                || EXT_F_IS_IN_RANGE<char>(ch, 'a', 'f')
                                || EXT_F_IS_IN_RANGE<char>(ch, 'A', 'F'))
                            {
                                contains_hex = true;
                                break;
                            }
                        }

                        size_t referer = 0;

                        if (contains_hex && !ref.empty())
                        {
                            referer = std::stoll(ref, nullptr, 16);
                            refs_set.insert(referer);

                            std::string desc = CUsermodeMemory::GetInstance()->heap_desc(referer);
                        }
                    }

                    for (auto& referer : refs_set)
                    {
                        auto ref_alloc = CUsermodeMemory::GetInstance()->get_alloc_for_addr(referer);

                        if (ref_alloc)
                        {
                            alloc->add_ref(ref_alloc);
                            ref_alloc->add_ptr(alloc);

                            bool exists = false;
                            for (auto& r_alloc : allocations)
                            {
                                if (r_alloc->equal(ref_alloc))
                                {
                                    exists = true;
                                    break;
                                }
                            }

                            if (!exists)
                                allocations.push_back(ref_alloc);
                        }
                    }
                }
            }

            s_cache[addr] = refs_set;
            s_cache_allocations[addr] = allocations;
        }
    }
    FC;

    return make_tuple(s_cache_allocations[addr], s_cache[addr]);
}

CUsermodeMemory::CUsermodeMemory()
{
    Analyze();
}

CUsermodeMemory::~CUsermodeMemory()
{
}

std::string CUsermodeMemory::heap_desc(size_t addr)
{
    for (auto& heap : m_heaps)
    {
        if (heap->in_range(addr))
            return heap->desc(addr);
    }

    return "";
}

std::string CUsermodeMemory::code_desc(size_t addr)
{
    for (auto& module : m_images)
    {
        if (module->in_range(addr) && module->is_code())
            return module->code_desc(addr);
    }

    return "";
}

std::string CUsermodeMemory::module_data_desc(size_t addr)
{
    for (auto& module : m_images)
    {
        if (module->in_range(addr))
            return module->data_desc(addr);
    }

    return "";
}

std::string CUsermodeMemory::stack_desc(size_t addr)
{
    for (auto& stack : m_stacks)
    {
        if (stack->in_range(addr) )
            return stack->desc(addr);
    }

    return "";
}

std::string CUsermodeMemory::heap_ref_by_desc(size_t addr)
{
    for (auto& heap : m_heaps)
    {
        if (heap->in_range(addr))
            return heap->ref_by_desc(addr);
    }

    return "";
}

std::tuple<std::vector<std::shared_ptr<CUsermodeHeapAllocation>>, std::set<size_t>> CUsermodeMemory::heap_ref_by(size_t addr)
{
    for (auto& heap : m_heaps)
    {
        if (heap->in_range(addr))
            return heap->ref_by(addr);
    }

    return make_tuple(std::vector<std::shared_ptr<CUsermodeHeapAllocation>>(), std::set<size_t>());
}

std::shared_ptr<CUsermodeHeapAllocation> CUsermodeMemory::get_allocation(size_t start, size_t end)
{
    for (auto& heap : m_heaps)
    {
        if (heap->in_range(start))
        {
            auto alloc = heap->get_allocation(start, end);
            if (alloc == nullptr)
            {
                alloc = std::make_shared<CUsermodeHeapAllocation>(start, end);

                heap->add_allocation(alloc);
            }

            return alloc;
        }
    }

    return nullptr;
}

std::shared_ptr<CUsermodeHeapAllocation> CUsermodeMemory::get_alloc_for_addr(size_t addr)
{
    for (auto& heap : m_heaps)
    {
        if (heap->in_range(addr))
        {
            auto alloc = heap->get_alloc_for_addr(addr);
            if (alloc != nullptr)
            {
                return alloc;
            }
        }
    }

    return nullptr;
}

void CUsermodeMemory::Analyze()
{
    static std::regex s_general_line(R"(^\+?\s+([0-9a-f`]+)\s+([0-9a-f`]+)\s+([0-9a-f`]+)\s+(.*)$)");

    static std::regex s_stack_usage(R"(.*\[~(\d+); ([0-9a-f]+)\.([0-9a-f]+)\])");

    static std::regex s_heap_usage(R"(.*\[ID: ([0-9a-f]+); Handle: ([0-9a-f]+); Type: (.+)\])");

    static std::regex s_image_usage(R"(.*\[(\w+);\s+(.*)\])");
    
    std::vector<std::string> mem_types = {"MEM_IMAGE", "MEM_PRIVATE", "MEM_MAPPED"};
    std::vector<std::string> mem_states = {"MEM_FREE", "MEM_RESERVE", "MEM_COMMIT"};
    std::vector<std::string> mem_protect = {"PAGE_NOACCESS", "PAGE_READONLY", "PAGE_READWRITE", "PAGE_GUARD", "PAGE_WRITECOMBINE", "PAGE_EXECUTE_READ", "PAGE_WRITECOPY"};

    std::string cmd = "!address";

    auto results = DK_X_CMD(cmd);
    for (auto result : results)
    {
        std::smatch sm;
        if (regex_match(result, sm, s_general_line))
        {
            if (sm.size() == 5)
            {
                std::string base_addr_str = sm[1].str();
                std::string end_addr_str = sm[2].str();
                std::string region_size_str = sm[3].str();
                std::string usage_str = sm[4].str();


                base_addr_str.erase(base_addr_str.find('`'), 1);
                end_addr_str.erase(end_addr_str.find('`'), 1);
                region_size_str.erase(region_size_str.find('`'), 1);


                size_t base = std::stoll(base_addr_str, nullptr, 16);
                size_t end = std::stoll(end_addr_str, nullptr, 16);
                size_t size = std::stoll(region_size_str, nullptr, 16);

                
                auto stack_pos = usage_str.find("Stack");
                auto heap_pos = usage_str.find("Heap");
                auto image_pos = usage_str.find("Image");


                if (stack_pos != std::string::npos)
                {
                    std::smatch sm_stack;
                    regex_match(usage_str, sm_stack, s_stack_usage);

                    if (sm_stack.size() == 4)
                    {
                        size_t thread_index = std::stoll(sm_stack[1], nullptr, 10);

                        size_t pid = std::stoll(sm_stack[2], nullptr, 16);
                        size_t tid = std::stoll(sm_stack[3], nullptr, 16);

                        m_stacks.push_back(make_unique<CUsermodeStack>(base, end, size, thread_index, tid, pid, usage_str));
                    }
                }
                else if (heap_pos != std::string::npos)
                {
                    std::smatch sm_heap;
                    regex_match(usage_str, sm_heap, s_heap_usage);

                    if (sm_heap.size() == 4)
                    {
                        size_t id = std::stoll(sm_heap[1], nullptr, 10);

                        size_t handle = std::stoll(sm_heap[2], nullptr, 16);
                        std::string type = sm_heap[3];

                        m_heaps.push_back(make_unique<CUsermodeHeap>(base, end, size, id, handle, type, usage_str));
                    }
                }
                else if (image_pos != std::string::npos)
                {
                    std::smatch sm_image;
                    regex_match(usage_str, sm_image, s_image_usage);

                    if (sm_image.size() == 3)
                    {
                        std::string name = sm_image[1];
                        std::string path = sm_image[2];

                        m_images.push_back(make_unique<CUsermodeModule>(base, end, size, name, path, usage_str));
                    }
                }
            }
            else
            {
                //EXT_F_OUT(" + Imperfect Match : %s\n", result.c_str());

                //for (auto i=0;i<sm.size();i++)
                //{
                //    EXT_F_OUT("part[ %d ], %s\n", i, sm[i].str().c_str());
                //}
            }
        }
        else
        {
            //EXT_F_OUT(" - No match: %s\n", result.c_str());
        }
    }
}

void CUsermodeMemory::analyze_mem(size_t addr, size_t len, size_t offset, bool b_recurse)
{
    size_t start = addr;

    while (len > 0x10000)
    {
        analyze_mem(start, 0x10000, offset, b_recurse);

        start += 0x10000;
        offset += 0x10000;
        len -= 0x10000;
    };

    try
    {
        std::vector<std::shared_ptr<CUsermodeHeapAllocation>> ref_allocs;

        uint8_t* buffer = new uint8_t[len];

        EXT_F_OUT("Analyzing memory in [ 0x%0I64x - 0x%0I64x ]\n", start, start + len);

        if (is_stack(addr))
        {
            EXT_F_DML("Which is Stack, %s\n", stack_desc(addr).c_str());
        }
        else if (is_code(addr))
        {
            EXT_F_DML("Which is Code, %s\n", code_desc(addr).c_str());
        }
        else if (is_module_data(addr))
        {
            EXT_F_DML("Which is Module Data, %s\n", module_data_desc(addr).c_str());
        }
        else if (is_heap(addr))
        {
            EXT_F_DML("Which is Heap, %s\n", heap_desc(addr).c_str());
        }

        size_t bytes_read = 0;
        HRESULT result = EXT_D_IDebugDataSpaces->ReadVirtual(start, buffer, (ULONG)len, (PULONG)&bytes_read);
        if (result != 0)
        {
            EXT_F_ERR("Fail to read memory in [ 0x%0I64x - 0x%0I64x ], HRESULT: 0x%08x\n", start, start + len, result);
            return;
        }

        std::stringstream ss;

        for (size_t i = 0; i < len / 8; i++)
        {
            ss.str("");
            size_t curr_qword = *(size_t*)(buffer + i * 8);

            size_t j = i + 1;

            if (curr_qword == 0)
            {
                while (j < len / 8 && *(size_t*)(buffer + j * 8) == 0)
                {
                    j++;
                };

                if (j - i >= ZERO_REPEAT_THREADHOLD)
                {
                    ss << std::hex << std::showbase << "+" << std::setw(18) << offset + i * 8 << ": \t" << std::setw(18) << curr_qword << " ";

                    ss << std::setw(18) << " [ 000 ] " << dump_plain_qword(curr_qword);

                    ss << " Zeros in range [ " << std::hex << std::showbase << offset + i * 8 << " - "
                        << offset + j * 8 << " )";

                    ss << " for " <<
                        (j - i) << " / " << std::showbase << std::dec << (j - i) << " times" <<
                        std::endl << "." << std::endl;

                    EXT_F_DML(ss.str().c_str());

                    i = j;

                    if (i >= len / 8)
                        break;

                    curr_qword = *(size_t*)(buffer + i * 8);
                    ss.str("");
                }
            }

            ss << std::hex << std::showbase << "+" << std::setw(18) << offset + i * 8 << ": \t" << std::setw(18) << curr_qword << " ";

            if (is_stack(curr_qword))
            {
                ss << std::setw(18) << " [ stack ] ";

                ss << dump_plain_qword(curr_qword);
                ss << " ";

                EXT_F_OUT(ss.str().c_str());

                EXT_F_DML(stack_desc(curr_qword).c_str());

                EXT_F_OUT("\n");
            }
            else if (is_heap(curr_qword))
            {
                ss << std::setw(18) << " [ heap ] ";

                ss << dump_plain_qword(curr_qword);
                ss << " ";

                EXT_F_OUT(ss.str().c_str());

                EXT_F_DML(heap_desc(curr_qword).c_str());

                EXT_F_OUT("\n");

                auto alloc = get_alloc_for_addr(curr_qword);
                if (alloc)
                {
                    bool b_exist = false;
                    for (auto ref_alloc : ref_allocs)
                    {
                        if (ref_alloc->equal(alloc))
                        {
                            b_exist = true;
                            break;
                        }
                    }

                    if (!b_exist)
                        ref_allocs.push_back(alloc);
                }
            }
            else if (is_code(curr_qword))
            {
                ss << std::setw(18) << " [ code ] ";

                ss << dump_plain_qword(curr_qword);
                ss << " ";

                EXT_F_OUT(ss.str().c_str());

                EXT_F_DML(code_desc(curr_qword).c_str());

                EXT_F_OUT("\n");
            }
            else if (is_global(curr_qword))
            {
                ss << std::setw(18) << " [ global ] ";

                ss << dump_plain_qword(curr_qword);
                ss << " ";

                EXT_F_OUT(ss.str().c_str());

                EXT_F_DML(module_data_desc(curr_qword).c_str());

                EXT_F_OUT("\n");
            }
            else if (is_readonly(curr_qword))
            {
                ss << std::setw(18) << " [ readonly ] ";

                ss << dump_plain_qword(curr_qword);
                ss << " ";

                EXT_F_OUT(ss.str().c_str());

                EXT_F_DML(module_data_desc(curr_qword).c_str());

                EXT_F_OUT("\n");
            }
            else if (curr_qword >= start && curr_qword < start + len)
            {
                ss << std::setw(18) << " [ this ] ";

                ss << dump_plain_qword(curr_qword);

                ss << std::endl;

                EXT_F_OUT(ss.str().c_str());
            }
            else
            {
                ss << std::setw(18) << " [     ] ";

                ss << dump_plain_qword(curr_qword);

                ss << std::endl;

                EXT_F_OUT(ss.str().c_str());
            }
        }

        if (b_recurse)
        {
            EXT_F_OUT("Referenced Heap Allocations:\n");

            for (auto& alloc : ref_allocs)
            {
                analyze_mem(alloc->start(), alloc->size(), alloc->start());
            }
        }
    }
    FC;
}



void CUsermodeMemory::analyze_mem_svg(size_t addr, size_t len, size_t offset, std::string svg_filename, bool b_recurse)
{
    size_t start = addr;

    while (len > 0x10000)
    {
        analyze_mem_svg(start, 0x10000, offset, svg_filename, b_recurse);

        start += 0x10000;
        offset += 0x10000;
        len -= 0x10000;
    };

    std::stringstream svg_base_filename_ss;

    if (svg_filename.size() > 4 && svg_filename.substr(svg_filename.size() - 4) == ".svg")
    {
        svg_base_filename_ss << svg_filename.substr(0, svg_filename.size() - 4)
            << "_" << std::hex << std::showbase << addr << ".svg";
    }
    else
    {
        svg_base_filename_ss << svg_filename << "_" << std::hex << std::showbase << addr << ".svg";
    }

    CHexSvg hex_svg(8);

    hex_svg.set_address(addr);

    try
    {
        //vector<shared_ptr<CUsermodeHeapAllocation>> ref_allocs;

        uint8_t* buffer = new uint8_t[len];

        size_t bytes_read = 0;
        HRESULT result = EXT_D_IDebugDataSpaces->ReadVirtual(start, buffer, (ULONG)len, (PULONG)&bytes_read);
        if (result != 0)
        {
            EXT_F_ERR("Fail to read memory in [ 0x%0I64x - 0x%0I64x ], HRESULT: 0x%08x\n", start, start + len, result);
            return;
        }

        auto highlights = CSvgTheme::GetInstance()->get_highlight();
        auto trivials = CSvgTheme::GetInstance()->get_trivial();

        std::string COLOR_HEAP       = highlights[0];
        std::string COLOR_STACK      = highlights[1];
        std::string COLOR_THIS       = highlights[2];
        std::string COLOR_CODE       = highlights[3];
        std::string COLOR_GLOBAL     = highlights[4];
        std::string COLOR_READONLY   = highlights[5];
        std::string COLOR_ZERO       = trivials[1];
        std::string COLOR_NORMAL     = trivials[0];


        hex_svg.add_legend(COLOR_HEAP, "Heap Pointer");
        hex_svg.add_legend(COLOR_STACK, "Stack Pointer");
        hex_svg.add_legend(COLOR_THIS, "This Pointer");
        hex_svg.add_legend(COLOR_CODE, "Code Pointer");
        hex_svg.add_legend(COLOR_GLOBAL, "Global Pointer");
        hex_svg.add_legend(COLOR_READONLY, "ReadOnly Pointer");
        hex_svg.add_legend(COLOR_ZERO, "Zero");
        hex_svg.add_legend(COLOR_NORMAL, "Normal");

        for (size_t i = 0; i < len / 8; i++)
        {
            size_t curr_qword = *(size_t*)(buffer + i * 8);

            if (curr_qword == 0)
            {
                hex_svg.add_buffer(qword_2_buffer(curr_qword), COLOR_ZERO);
            }
            else if (is_stack(curr_qword))
            {
                hex_svg.add_buffer(qword_2_buffer(curr_qword), COLOR_STACK);
            }
            else if (is_heap(curr_qword))
            {
                hex_svg.add_buffer(qword_2_buffer(curr_qword), COLOR_HEAP);
            }
            else if (is_code(curr_qword))
            {
                hex_svg.add_buffer(qword_2_buffer(curr_qword), COLOR_CODE);
            }
            else if (is_global(curr_qword))
            {
                hex_svg.add_buffer(qword_2_buffer(curr_qword), COLOR_GLOBAL);
            }
            else if (is_readonly(curr_qword))
            {
                hex_svg.add_buffer(qword_2_buffer(curr_qword), COLOR_READONLY);
            }
            else if (curr_qword >= start && curr_qword < start + len)
            {
                hex_svg.add_buffer(qword_2_buffer(curr_qword), COLOR_THIS);
            }
            else
            {
                hex_svg.add_buffer(qword_2_buffer(curr_qword), COLOR_NORMAL);
            }
        }

        //if (b_recurse)
        //{
        //    EXT_F_OUT("Referenced Heap Allocations:\n");

        //    for (auto& alloc : ref_allocs)
        //    {
        //        analyze_mem(alloc->start(), alloc->size(), alloc->start());
        //    }
        //}
    }
    FC;

    std::ofstream svg_ss(svg_base_filename_ss.str(), std::ios::out | std::ios::trunc);
    svg_ss << hex_svg.generate();
}

void CUsermodeMemory::analyze_qword(size_t curr_qword)
{
    std::stringstream ss;
    ss << ": \t" << std::setw(18) << curr_qword << " ";

    if (is_stack(curr_qword))
    {
        ss << std::setw(18) << " [ stack ] ";

        ss << dump_plain_qword(curr_qword);
        ss << " ";

        EXT_F_OUT(ss.str().c_str());

        EXT_F_DML(stack_desc(curr_qword).c_str());

        EXT_F_OUT("\n");
    }
    else if (is_heap(curr_qword))
    {
        ss << std::setw(18) << " [ heap ] ";

        ss << dump_plain_qword(curr_qword);
        ss << " ";

        EXT_F_OUT(ss.str().c_str());

        EXT_F_DML(heap_desc(curr_qword).c_str());

        EXT_F_OUT("\n");
    }
    else if (is_code(curr_qword))
    {
        ss << std::setw(18) << " [ code ] ";

        ss << dump_plain_qword(curr_qword);
        ss << " ";

        EXT_F_OUT(ss.str().c_str());

        EXT_F_DML(code_desc(curr_qword).c_str());

        EXT_F_OUT("\n");
    }
    else if (is_global(curr_qword))
    {
        ss << std::setw(18) << " [ global ] ";

        ss << dump_plain_qword(curr_qword);
        ss << " ";

        EXT_F_OUT(ss.str().c_str());

        EXT_F_DML(module_data_desc(curr_qword).c_str());

        EXT_F_OUT("\n");
    }
    else if (is_readonly(curr_qword))
    {
        ss << std::setw(18) << " [ readonly ] ";

        ss << dump_plain_qword(curr_qword);
        ss << " ";

        EXT_F_OUT(ss.str().c_str());

        EXT_F_DML(module_data_desc(curr_qword).c_str());

        EXT_F_OUT("\n");
    }
    else
    {
        ss << std::setw(18) << " [     ] ";

        ss << dump_plain_qword(curr_qword);

        ss << std::endl;

        EXT_F_OUT(ss.str().c_str());
    }
}

void CUsermodeMemory::analyze_mem_around(size_t addr)
{
    analyze_mem((addr - 0x40) & 0xFFFFFFFFFFF0, 0x80, (addr - 0x40) & 0xFFFFFFFFFFF0);
}

void CUsermodeMemory::analyze_ref_by(size_t addr)
{
    if (is_stack(addr))
    {
        EXT_F_DML("[-] Stack, %s\n", stack_desc(addr).c_str());
        return;
    }
    else if (is_code(addr))
    {
        EXT_F_DML("[-] Code, %s\n", code_desc(addr).c_str());
        return;
    }
    else if (is_module_data(addr))
    {
        EXT_F_DML("[-] Module Data, %s\n", module_data_desc(addr).c_str());
        return;
    }
    else if (is_heap(addr))
    {
        EXT_F_OUT("[+] Heap\n");

        auto refer_tuple = heap_ref_by(addr);

        auto refer_set = get<1>(refer_tuple);

        for (auto ref_alloc : get<0>(refer_tuple))
        {
            std::stringstream ss;

            ss << DML_CMD << "!dk uaddr_ref_by " << std::hex << std::showbase << ref_alloc->start()
                << DML_TEXT << "    [+] Heap allocation " << std::hex << std::showbase << ref_alloc->start() << " - " << ref_alloc->end()
                << DML_END
                << std::endl;

            EXT_F_STR_DML(ss);

            analyze_mem(ref_alloc->start(), ref_alloc->size(), ref_alloc->start());
        }

        for (auto& refer : refer_set)
        {
            if (is_stack(refer))
            {
                EXT_F_DML("    [-] Stack, %s\n", stack_desc(refer).c_str());
                return;
            }
            else if (is_code(refer))
            {
                EXT_F_DML("    [-] Code, %s\n", code_desc(refer).c_str());
                return;
            }
            else if (is_module_data(refer))
            {
                EXT_F_DML("    [-] Module Data, %s\n", module_data_desc(refer).c_str());
                return;
            }
            else if (is_heap(refer))
            {
            }
            else
            {
                EXT_F_OUT("[Unknown Addr 0x%0I64x]\n", refer);
                return;
            }
        }
    }
    else
    {
        EXT_F_OUT("[Unknown Addr 0x%0I64x]\n", addr);
        return;
    }

    return;
}

void CUsermodeMemory::analyze_ref_tree(size_t addr, size_t level)
{
    static std::vector<std::shared_ptr<CUsermodeHeapAllocation>> s_heap_stacks;

    if (level > m_max_ref_by_tree_levels)
        return;

    std::string level_str(level * 4, '-');
    std::string next_level_str((level + 1) * 4, '-');

    if (is_stack(addr))
    {
        EXT_F_DML("%s [-] Stack, %s\n", level_str.c_str(), stack_desc(addr).c_str());
        return;
    }
    else if (is_code(addr))
    {
        EXT_F_DML("%s [-] Code, %s\n", level_str.c_str(), code_desc(addr).c_str());
        return;
    }
    else if (is_module_data(addr))
    {
        EXT_F_DML("%s [-] Module Data, %s\n", level_str.c_str(), module_data_desc(addr).c_str());
        return;
    }
    else if (is_heap(addr))
    {
        EXT_F_OUT("%s [+] Heap, ", level_str.c_str());
        std::string desc = heap_desc(addr);
        if (desc.empty())
            EXT_F_OUT("0x%0I64x\n", addr);
        else
            EXT_F_DML("%s\n", desc.c_str());

        s_heap_stacks.push_back(get_alloc_for_addr(addr));

        auto refer_tuple = heap_ref_by(addr);

        auto refer_set = get<1>(refer_tuple);

        for (auto ref_alloc : get<0>(refer_tuple))
        {
            bool b_nested = false;
            for (auto upper_node : s_heap_stacks)
            {
                if (ref_alloc->equal(upper_node))
                {
                    b_nested = true;
                    break;
                }
            }

            if (!b_nested)
                analyze_ref_tree(ref_alloc->start(), level + 1);
            else
                EXT_F_OUT("%s [-] Heap loop, 0x%0I64x - 0x%0I64x\n", next_level_str.c_str(), ref_alloc->start(), ref_alloc->end());
        }
        s_heap_stacks.pop_back();

        for (auto& refer : refer_set)
        {
            if (is_stack(refer))
            {
                EXT_F_DML("%s [-] Stack, %s\n", next_level_str.c_str(), stack_desc(refer).c_str());
                return;
            }
            else if (is_code(refer))
            {
                EXT_F_DML("%s [-] Code, %s\n", next_level_str.c_str(), code_desc(refer).c_str());
                return;
            }
            else if (is_module_data(refer))
            {
                EXT_F_DML("%s [-] Module Data, %s\n", next_level_str.c_str(), module_data_desc(refer).c_str());
                return;
            }
            else if (is_heap(refer))
            {
            }
            else
            {
                EXT_F_OUT("%s [-] Unrecognized 0x%0I64x\n", next_level_str.c_str(), refer);
                return;
            }
        }

    }
    else
    {
        EXT_F_OUT("%s [-] Unrecognized 0x%0I64x\n", level_str.c_str(), addr);
        return;
    }

    return;
}

void CUsermodeMemory::dump_all_stacks()
{
    for (auto& stack : m_stacks)
    {
        auto stack_range = stack->dump();

        analyze_mem(get<0>(stack_range), get<1>(stack_range), get<0>(stack_range));
    }
}

CUsermodeHeapAllocation::CUsermodeHeapAllocation(size_t start, size_t end)
    :m_start(start)
    ,m_end(end)
    ,m_size(end - start)
{
}

CUsermodeHeapAllocation::~CUsermodeHeapAllocation()
{
}
