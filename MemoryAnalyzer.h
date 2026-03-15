#pragma once

#include <string>
#include <map>
#include <tuple>



class CMemoryAnalyzer
{
public:
    CMemoryAnalyzer(std::string data, size_t addr, size_t len, size_t local_start = 0, size_t local_end = 0)
        :m_data(data)
        ,m_addr(addr)
        , m_len(len)
    {
        if (m_data.size() < m_len)
            m_len = m_data.size();

        if (local_start == 0)
            m_local_start = m_addr;
        else
            m_local_start = local_start;

        if (local_end == 0)
            m_local_end = m_local_start + m_len;
        else
            m_local_end = local_end;
			
        try
        {
            m_heap = DK_GET_HEAP();
        }
        catch (std::exception e)
        {
            m_heap.clear();
        };
        
    }

    size_t is_in_cur_stack(size_t addr, size_t id=0);

    bool is_local(size_t addr);

    std::string get_symbol(size_t addr);

    std::tuple<size_t, size_t> is_heap(size_t addr);

    void carve_strings();

    void carve_ustrings();

    void analyze();

    std::map<size_t, size_t> get_ptr2local();
    std::map<size_t, size_t> get_ptr2stack();
    std::map<size_t, std::tuple<std::string, size_t>> get_ptr2sym();
    std::map<size_t, std::tuple<size_t, size_t, size_t>> get_ptr2heap();

    std::map<size_t, std::string> get_ptr2astr();
    std::map<size_t, std::string> get_ptr2ustr();


private:
    std::string m_data;
    size_t m_addr;
    size_t m_len;

    size_t m_local_start;
    size_t m_local_end;
    std::map<size_t, size_t> m_ptr2locals;
    std::map<size_t, size_t> m_ptr2stacks;
    std::map<size_t, std::string> m_ptr2astrs;
    std::map<size_t, std::string> m_ptr2ustrs;
    std::map<size_t, std::tuple<std::string, size_t>> m_ptr2syms;
    std::map<size_t, std::tuple<size_t, size_t, size_t>> m_ptr2heaps;

    std::vector<ttd_heap_memory> m_heap;
};