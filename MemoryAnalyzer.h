#pragma once

#include <string>
#include <map>
#include <tuple>



class CMemoryAnalyzer
{
public:
    CMemoryAnalyzer(std::string data, size_t addr, size_t len)
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

    std::string get_symbol(size_t addr);

    std::tuple<size_t, size_t> is_heap(size_t addr);

    void analyze();

    std::map<size_t, size_t> get_ptr2local();
    std::map<size_t, size_t> get_ptr2stack();
    std::map<size_t, std::tuple<std::string, size_t>> get_ptr2sym();
    std::map<size_t, std::tuple<size_t, size_t, size_t>> get_ptr2heap();

private:
    std::string m_data;
    size_t m_addr;
    size_t m_len;

    std::map<size_t, size_t> m_ptr2locals;
    std::map<size_t, size_t> m_ptr2stacks;
    std::map<size_t, std::tuple<std::string, size_t>> m_ptr2syms;
    std::map<size_t, std::tuple<size_t, size_t, size_t>> m_ptr2heaps;

    std::vector<ttd_heap_memory> m_heap;
};