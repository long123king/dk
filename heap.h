#pragma once

#include "CmdInterface.h"

#include <map>
#include <set>

DECLARE_CMD(heap_ssum)
DECLARE_CMD(heap_bysize)
DECLARE_CMD(heap_oversize)

void heap_ssum();
void heap_by_size(size_t size);
void heap_over_size(size_t size);

class CChunkInfo
{
public:
    CChunkInfo(size_t _size, string _flags)
        :size(_size)
        , flags(_flags)
    {
    }

    size_t size;
    string flags;
};

class CHeapSummary
{ 
public:
    static CHeapSummary* GetInstance()
    {
        static CHeapSummary s_instance;
        return &s_instance;
    }

    CHeapSummary();
    ~CHeapSummary();

    void Analyze();

    void add_chunk(string& line);

    void dump_size_summary();

    void dump_size(size_t size);
    void dump_size_over(size_t size);

private:                 
    //map<size_t, unique_ptr<CChunkInfo>> m_chunks;
    map<size_t, set<size_t>> m_size_map;
};
