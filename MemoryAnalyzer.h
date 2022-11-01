#pragma once

#include <string>
#include <map>
#include <tuple>

using namespace std;

class CMemoryAnalyzer
{
public:
    CMemoryAnalyzer(string data, size_t addr, size_t len)
        :m_data(data)
        ,m_addr(addr)
        , m_len(len)
    {
        if (m_data.size() < m_len)
            m_len = m_data.size();

        m_heap = DK_GET_HEAP();
    }

    size_t is_in_cur_stack(size_t addr);

    bool is_local(size_t addr);

    string get_symbol(size_t addr);

    tuple<size_t, size_t> is_heap(size_t addr);

    void analyze();

    map<size_t, size_t> get_ptr2local();
    map<size_t, size_t> get_ptr2stack();
    map<size_t, tuple<string, size_t>> get_ptr2sym();
    map<size_t, tuple<size_t, size_t, size_t>> get_ptr2heap();

private:
    string m_data;
    size_t m_addr;
    size_t m_len;

    map<size_t, size_t> m_ptr2locals;
    map<size_t, size_t> m_ptr2stacks;
    map<size_t, tuple<string, size_t>> m_ptr2syms;
    map<size_t, tuple<size_t, size_t, size_t>> m_ptr2heaps;

    vector<ttd_heap_memory> m_heap;
};