#include "model.h"
#include "heap.h"
#include "CmdExt.h"
#include "CmdList.h"


DEFINE_CMD(heap_ssum)
{
    if (!DK_MODEL_ACCESS->isUsermode())
    {
        EXT_F_OUT("Usage: !dk heap_ssum\nUser Mode Only\n");
        return;
    }

    heap_ssum();
}

DEFINE_CMD(heap_bysize)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 2)
    {
        EXT_F_OUT("Usage: !dk heap_bysize <size_in_hex>\nUser Mode Only\n");
        return;
    }

    size_t size = EXT_F_IntArg(args, 1, 0);

    heap_by_size(size);
}

DEFINE_CMD(heap_oversize)
{
    if (!DK_MODEL_ACCESS->isUsermode() || args.size() < 2)
    {
        EXT_F_OUT("Usage: !dk heap_oversize <size_in_hex>\nUser Mode Only\n");
        return;
    }

    size_t size = EXT_F_IntArg(args, 1, 0);

    heap_over_size(size);
}

void heap_ssum()
{
    CHeapSummary::GetInstance()->dump_size_summary();
}

void heap_by_size(size_t size)
{
    CHeapSummary::GetInstance()->dump_size(size);
}

void heap_over_size(size_t size)
{
    CHeapSummary::GetInstance()->dump_size_over(size);
}

CHeapSummary::CHeapSummary()
{
    Analyze();
}

CHeapSummary::~CHeapSummary()
{
}

void CHeapSummary::Analyze()
{
    EXT_F_OUT("analyzing all process heaps, it may take time, please wait in patience...\n");

    string cmd = "!heap -s -a -v";

    auto results = DK_X_CMD(cmd);
    for (auto& result : results)
    {
        if (result.size() > 0x66 && result[1] == '0')
        {
            if (result[0] == '.')
                result = result.substr(1);

            add_chunk(result);
        }
    }
}

void CHeapSummary::add_chunk(string& line)
{
    size_t addr = 0;
    size_t size = 0;

    addr = stoull(line.c_str(), nullptr, 16);
    //addr = stoull(line.substr(0, 16), nullptr, 16);
    size = stoull(line.c_str() + line.find_first_not_of(' ', 0x48), nullptr, 16);
    //size = stoull(line.substr(line.find_first_not_of(' ', 0x48)), nullptr, 16);

    //auto flag_str = line.substr(0x66);

    //if (*flag_str.rbegin() == '\n')
    //    flag_str.resize(flag_str.size() - 1);

    //if (m_chunks.find(addr) == m_chunks.end())
    //    m_chunks[addr] = make_unique<CChunkInfo>(size, flag_str);

    if (m_size_map.find(size) == m_size_map.end())
        m_size_map[size] = set<size_t>();

    m_size_map[size].insert(addr);
}

void CHeapSummary::dump_size_summary()
{
    for (auto it : m_size_map)
    {
        stringstream ss;
        ss << "[+] " << hex << showbase << it.first << " : "
            << it.second.size() << " chunks" << endl;

        //for (auto itt : it.second)
        //{
        //    ss << "    [-] " << hex << showbase << itt << endl;
        //}

        //ss << endl;

        EXT_F_STR_OUT(ss);
    }
}

void CHeapSummary::dump_size(size_t size)
{
    auto it = m_size_map.find(size);
    if (it != m_size_map.end())
    {
        stringstream ss;
        ss << "[+] " << hex << showbase << it->first << " : "
            << it->second.size() << " chunks" << endl;

        for (auto itt : it->second)
        {
            ss << "    [-] " << hex << showbase << itt << endl;
        }

        ss << endl;

        EXT_F_STR_OUT(ss);
    }
}

void CHeapSummary::dump_size_over(size_t size)
{
    for (auto it : m_size_map)
    {
        if (it.first >= size)
        {
            stringstream ss;
            ss << "[+] " << hex << showbase << it.first << " : "
                << it.second.size() << " chunks" << endl;

            for (auto itt : it.second)
            {
                ss << "    [-] " << hex << showbase << itt << endl;
            }

            ss << endl;

            EXT_F_STR_OUT(ss);
        }
    }
}
