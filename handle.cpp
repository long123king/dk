#include "model.h"
#include "handle.h"
#include "CmdExt.h"
#include "CmdList.h"
#include "object.h"

#include <iomanip>

#pragma warning( disable : 4244)

CHandleTable::CHandleTable(
    __in size_t levels,
    __in size_t l1Addr,
    __in size_t handleCount)
    :m_levels(levels)
    , m_l1Addr(l1Addr)
    , m_handleCount(handleCount)
{
}


CHandleTable::~CHandleTable()
{
}

void
CHandleTable::traverse(
    __in std::function<bool(size_t, size_t)> callback
)
{
    if (0 == m_levels)
    {
        uint8_t* this_page = new uint8_t[0x1000];
        if (S_OK == g_ExtInstancePtr->m_Data->ReadVirtual(m_l1Addr, this_page, 0x1000, NULL))
        {
            for (size_t i = 0; i < 256 && m_handleCount > 0; i++)
            {
                size_t entry = *(size_t*)(this_page + i * 0x10);
                size_t access = *(size_t*)(this_page + i * 0x10 + 0x08);

                callback(entry, access);
                m_handleCount--;
            }
        }

        delete[] this_page;

        //for (size_t i = 0; i < 256 && m_handleCount > 0; i++)
        //{
        //    size_t entry = 0;
        //    size_t access = 0;
        //    
        //    if (g_ExtInstancePtr &&
        //        S_OK == g_ExtInstancePtr->m_Data->ReadVirtual(m_l1Addr + i * 0x10, &entry, sizeof(size_t), NULL) &&
        //        S_OK == g_ExtInstancePtr->m_Data->ReadVirtual(m_l1Addr + i * 0x10 + 0x08, &access, sizeof(size_t), NULL) &&
        //        callback(entry, access))
        //        m_handleCount--;
        //        
        //}
    }
    else
    {
        uint8_t* this_page = new uint8_t[0x1000];
        if (S_OK == g_ExtInstancePtr->m_Data->ReadVirtual(m_l1Addr, this_page, 0x1000, NULL))
        {
            for (size_t i = 0; i < 512; i++)
            {
                size_t next_level_addr = *(size_t*)(this_page + i * 0x08);

                CHandleTable next_level_table(m_levels - 1, next_level_addr, m_handleCount);
                next_level_table.traverse(callback);
                m_handleCount = next_level_table.remained();
                if (m_handleCount == 0)
                    break;
            }
        }

        delete[] this_page;

        //for (size_t i = 0; i < 512; i++)
        //{
        //    size_t next_level_addr = 0;
        //    if (g_ExtInstancePtr &&
        //        S_OK == g_ExtInstancePtr->m_Data->ReadVirtual(m_l1Addr + i * 0x08, &next_level_addr, sizeof(size_t), NULL))
        //    {
        //        //size_t processed_count = i * 256 * (1 << ((m_levels - 1 >= 0 ? m_levels - 1 : 0) * 8));
        //        CHandleTable next_level_table(m_levels - 1, next_level_addr, m_handleCount);
        //        next_level_table.traverse(callback);    
        //        m_handleCount = next_level_table.remained();
        //        if (m_handleCount == 0)
        //            break;
        //    }
        //}
    }
}

void dump_handle_table(size_t handle_table_addr)
{
    size_t table_code = 0;
    size_t handle_count = 0;

    try
    {
        ExtRemoteTyped handle_table("(nt!_HANDLE_TABLE*)@$extin", handle_table_addr);
        table_code = handle_table.Field("TableCode").GetLongPtr();

        size_t freelist_count = EXT_F_READ<size_t>(EXT_F_Sym2Addr("nt!ExpFreeListCount"));

        if (freelist_count == 0)
        {
            auto free_list1 = handle_table.Field("FreeLists").ArrayElement(0);
            handle_count = free_list1.Field("HandleCount").GetUlong();
        }
        else
        {
            for (auto i = 0; i < freelist_count; i++)
            {
                auto free_listN = handle_table.Field("FreeLists").ArrayElement(i);
                handle_count += free_listN.Field("HandleCount").GetUlong();
            }
        }
    }
    FC;

    size_t level = table_code & 0x00000000000000FF;
    size_t l1_addr = table_code & 0xFFFFFFFFFFFFFF00;

    CHandleTable handle_table(level, l1_addr, handle_count);

    EXT_F_OUT("\n[dk:] handle table addr: 0x%016X, level: %d, handle count: 0x%016X\n", l1_addr, level, handle_count);

    //Out("HandleTable : 0x%I64x, count: 0x%08x, level: 0x%08x\n", handle_table_addr, handle_count, level);
    //Out("%-18s %-18s %-10s %-10s %-4s %-20s %s\n", "object_table_entry", "object_header_addr", "access", "handle", "type", "type_name", "object_name");
    size_t handle_value = 0;
    handle_table.traverse([&handle_value](size_t entry, size_t access) {
        //static size_t handle_value = 0;
        bool bRet = false;

        if ((entry & 0x01) != 0 &&
            (entry & 0x8000000000000000) != 0)
        {
            size_t addr = ((entry >> 0x10) | 0xFFFF000000000000) & 0xFFFFFFFFFFFFFFF0;

            if (EXT_F_ValidAddr(addr))
            {
                try
                {
                    ExtRemoteTyped obj_header("(nt!_OBJECT_HEADER*)@$extin", addr);
                    uint8_t r_index = real_index(obj_header.Field("TypeIndex").GetUchar(), addr);

                    std::wstring obj_name = dump_obj_name(addr);
                    std::wstring type_name = getTypeName(r_index);

                    std::string str_type_name(type_name.begin(), type_name.end());
                    std::string str_obj_name(obj_name.begin(), obj_name.end());

                    std::stringstream ss;

                    ss << std::hex << std::showbase
                        << std::setw(18) << entry << " ";

                    ss << "<link cmd=\"!object ";

                    ss << addr + 0x30 << "\">" << std::setw(18) << addr << "</link> "
                        << "<link cmd=\"!dk obj " << addr << "\">detail</link> "
                        << std::setw(10) << access << " "
                        << std::hex << std::setw(8) << handle_value << /*(" << setw(8) << dec << noshowbase << handle_value << hex << showbase << ")*/" "
                        << std::setw(20) << str_type_name << " [" << std::setw(4) << (uint16_t)r_index << "]   ";

                    if (obj_name.empty() && type_name == L"File")
                    {
                        std::wstring file_name = dump_file_name(addr + 0x30);
                        std::string str_file_name(file_name.begin(), file_name.end());

                        ss << str_file_name;
                    }
                    else
                        ss << str_obj_name;

                    ss << std::endl;

                    EXT_F_DML(ss.str().c_str());
                }
                FC;
            }
            else
                EXT_F_OUT("----> Invalid addr at 0x%0I64x\n", addr);

            bRet = true;
        }

        handle_value += 4;

        return bRet;
        });
}

DEFINE_CMD(handle_table)
{
    if (!DK_MODEL_ACCESS->isKernelmode() || args.size() < 2)
    {
        EXT_F_OUT("Usage: !dk handle_table <handle_table_addr>\nKernel-Mode Only\n");
        return;
    }

    size_t handle_table_addr = EXT_F_IntArg(args, 1, 0);
    dump_handle_table(handle_table_addr);
}

DEFINE_CMD(khandles)
{
    if (!DK_MODEL_ACCESS->isKernelmode())
    {
        EXT_F_OUT("Usage: !dk khandles\nKernel-Mode Only\n");
        return;
    }
    dump_kernel_handle_table();
}

void dump_kernel_handle_table()
{
    try
    {
        size_t kernel_handle_table_addr = EXT_F_Sym2Addr("nt!ObpKernelHandleTable");
        dump_handle_table(EXT_F_READ<size_t>(kernel_handle_table_addr));
    }
    FC
}

void dump_process_handle_table(size_t process_addr)
{
    try
    {
        ExtRemoteTyped curr_eprocess("(nt!_EPROCESS*)@$extin", process_addr);
        size_t handle_table_addr = curr_eprocess.Field("ObjectTable").GetLongPtr();
        dump_handle_table(handle_table_addr);
    }
    FC
}