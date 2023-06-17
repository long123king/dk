#pragma once

#include "CmdInterface.h"

#include <set>

DECLARE_CMD(usearch_astr)
DECLARE_CMD(usearch_ustr)
DECLARE_CMD(usearch_bytes)
DECLARE_CMD(usearch_addr)
DECLARE_CMD(uaddr_ref_disp)
DECLARE_CMD(uaddr_ref_by)
DECLARE_CMD(uaddr_ref_tree)
DECLARE_CMD(uaddr_analyze)
DECLARE_CMD(uaddr_analyze_svg)
DECLARE_CMD(uaddr_analyze2)
DECLARE_CMD(ustacks)
DECLARE_CMD(uregs)



void usermode_search_astr(string pattern, size_t level);
void usermode_search_ustr(string pattern, size_t level);
void usermode_search_bytes(string bytes, size_t level);
void usermode_search_addr(size_t addr, size_t level);
void usermode_addr_ref_disp(size_t start, size_t len, size_t ref_start, size_t ref_len, size_t ref_disp);
void usermode_addr_ref_by(size_t start);
void usermode_addr_ref_tree(size_t start, size_t level);
void usermode_addr_analyze(size_t start, size_t len, size_t offset);
void usermode_addr_analyze_svg(size_t start, size_t len, string svg_filename, size_t offset);
void usermode_addr_analyze2(size_t start, size_t len, size_t offset);

void usermode_stacks();
void usermode_regs();

class CUsermodeStack
{
public:
    CUsermodeStack(size_t start, size_t end, size_t size, size_t t_index, size_t tid, size_t pid, string usage);

    ~CUsermodeStack();

    void Analyze();

    bool in_range(size_t addr)
    {
        return addr >= m_start && addr < m_end;
    }

    string desc(size_t addr);

    tuple<size_t, size_t> dump();
private:
    size_t m_start{ 0 };
    size_t m_end{ 0 };
    size_t m_size{ 0 };
    size_t m_t_index{ 0 };
    size_t m_tid{ 0 };
    size_t m_pid{ 0 };

    string m_usage_str;
};

class CUsermodeModule
{
public:
    CUsermodeModule(size_t start, size_t end, size_t size, string name, string path, string usage);
    ~CUsermodeModule();

    void Analyze();

    bool in_range(size_t addr)
    {
        return addr >= m_start && addr < m_end;
    }

    bool is_code()
    {
        return m_usage_str.find("PAGE_EXECUTE_READ") != string::npos;
    }

    bool is_readonly()
    {
        return m_usage_str.find("PAGE_READONLY") != string::npos;
    }

    bool is_global()
    {
        return m_usage_str.find("PAGE_READWRITE") != string::npos || 
            m_usage_str.find("PAGE_WRITECOPY") != string::npos;
    }

    string code_desc(size_t addr);

    string data_desc(size_t addr);


private:
    size_t m_start{ 0 };
    size_t m_end{ 0 };
    size_t m_size{ 0 };
    string m_name{ 0 };
    string m_path{ 0 };

    string m_usage_str;
};

class CUsermodeHeapAllocation
{
public: 
    CUsermodeHeapAllocation(size_t start, size_t end);
    ~CUsermodeHeapAllocation();

    bool in_range(size_t addr)
    {
        return addr >= m_start && addr < m_end;
    }

    bool equal(shared_ptr<CUsermodeHeapAllocation> other)
    {
        return other && m_start == other->start() && m_end == other->end();
    }

    void add_ref(shared_ptr<CUsermodeHeapAllocation> ref)
    {
        for (auto& it_ref : m_references)
        {
            if (it_ref->equal(ref))
                return;
        }

        m_references.push_back(ref);
    }

    void add_ptr(shared_ptr<CUsermodeHeapAllocation> ptr)
    {
        for (auto& it_ptr : m_pointers)
        {
            if (it_ptr->equal(ptr))
                return;
        }

        m_pointers.push_back(ptr);
    }

    size_t start()
    {
        return m_start;
    }

    size_t end()
    {
        return m_end;
    }

    size_t size()
    {
        return m_size;
    }

    void set_desc(string desc)
    {
        m_desc = desc;
    }

    string desc()
    {
        return m_desc;
    }

private:
    size_t m_start;
    size_t m_end;
    size_t m_size;

    string m_desc;

    vector<shared_ptr<CUsermodeHeapAllocation>> m_pointers;
    vector<shared_ptr<CUsermodeHeapAllocation>> m_references;
};

class CUsermodeHeap
{
public: 
    CUsermodeHeap(size_t start, size_t end, size_t size, size_t id, size_t handle, string type, string usage);
    ~CUsermodeHeap();

    void Analyze();

    bool in_range(size_t addr)
    {
        return addr >= m_start && addr < m_end;
    }

    string desc(size_t addr);

    string ref_by_desc(size_t addr);

    tuple<vector<shared_ptr<CUsermodeHeapAllocation>>, set<size_t>> ref_by(size_t addr);

    shared_ptr<CUsermodeHeapAllocation> get_allocation(size_t start, size_t end)
    {
        for (auto& alloc : m_allocations)
        {
            if (alloc->start() == start && alloc->end() == end)
                return alloc;
        }

        return nullptr;
    }

    void add_allocation(shared_ptr<CUsermodeHeapAllocation> alloc)
    {
        m_allocations.push_back(alloc);
    }

    shared_ptr<CUsermodeHeapAllocation> get_alloc_for_addr(size_t addr)
    {
        for (auto& alloc : m_allocations)
        {
            if (alloc->start() <= addr && alloc->end() > addr)
                return alloc;
        }

        return nullptr;
    }

private:
    size_t m_start{ 0 };
    size_t m_end{ 0 };
    size_t m_size{ 0 };
    size_t m_id{ 0 };
    size_t m_handle{ 0 };
    string m_type{ 0 };

    string m_usage_str;

    vector<shared_ptr<CUsermodeHeapAllocation>> m_allocations;

};

class CUsermodeMemory
{
public:
    static CUsermodeMemory* GetInstance()
    {
        static CUsermodeMemory s_instance;

        return &s_instance;
    }

    CUsermodeMemory();
    ~CUsermodeMemory();

    void Analyze();

    bool is_stack(size_t addr)
    {
        for (auto& stack : m_stacks)
        {
            if (stack->in_range(addr))
                return true;
        }

        return false;
    }

    bool is_heap(size_t addr)
    {
        for (auto& heap : m_heaps)
        {
            if (heap->in_range(addr))
                return true;
        }

        return false;
    }

    bool is_code(size_t addr)
    {
        for (auto& module : m_images)
        {
            if (module->in_range(addr) && module->is_code())
                return true;
        }

        return false;
    }

    bool is_readonly(size_t addr)
    {
        for (auto& module : m_images)
        {
            if (module->in_range(addr) && module->is_readonly())
                return true;
        }

        return false;
    }

    bool is_global(size_t addr)
    {
        for (auto& module : m_images)
        {
            if (module->in_range(addr) && module->is_global())
                return true;
        }

        return false;
    }

    bool is_module_data(size_t addr)
    {
        for (auto& module : m_images)
        {
            if (module->in_range(addr) && !module->is_code())
                return true;
        }

        return false;
    }

    string heap_desc(size_t addr);

    string code_desc(size_t addr);

    string module_data_desc(size_t addr);

    string stack_desc(size_t addr);

    string heap_ref_by_desc(size_t addr);

    tuple<vector<shared_ptr<CUsermodeHeapAllocation>>, set<size_t>> heap_ref_by(size_t addr);

    shared_ptr<CUsermodeHeapAllocation> get_allocation(size_t start, size_t end);

    shared_ptr<CUsermodeHeapAllocation> get_alloc_for_addr(size_t addr);

    void analyze_mem(size_t addr, size_t len, size_t offset, bool b_recurse = false);

    void analyze_mem_svg(size_t addr, size_t len, size_t offset, string svg_filename, bool b_recurse = false);

    void analyze_qword(size_t qword);

    void analyze_mem_around(size_t addr);

    void analyze_ref_by(size_t addr);

    void analyze_ref_tree(size_t addr, size_t level = 0);

    void dump_all_stacks();

    void dummy()
    {
    }

    string qword_2_buffer(size_t qw)
    {
        string str(8, '0');
        memcpy(str.data(), &qw, 8);
        return str;
    }

    void set_ref_by_tree_levels(size_t level)
    {
        m_max_ref_by_tree_levels = level;
    }

private:
    vector<unique_ptr<CUsermodeStack>> m_stacks;
    vector<unique_ptr<CUsermodeHeap>> m_heaps;
    vector<unique_ptr<CUsermodeModule>> m_images;

    size_t m_max_ref_by_tree_levels{ 10 };
};
