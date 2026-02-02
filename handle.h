#pragma once

#include "CmdInterface.h"
#include <functional>

DECLARE_CMD(handle_table);
DECLARE_CMD(khandles);

class CHandleTable
{
public:
    CHandleTable(
        __in size_t levels,
        __in size_t l1Addr,
        __in size_t handleCount);
    ~CHandleTable();

    void
        traverse(
            __in std::function<bool(size_t, size_t)> callback
        );

    size_t
        remained()
    {
        return m_handleCount;
    }

private:
    size_t m_levels;
    size_t m_l1Addr;
    size_t m_handleCount;
};

void dump_kernel_handle_table();
void dump_process_handle_table(size_t process_addr);