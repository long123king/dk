#include "pool.h"
#include "CmdExt.h"
#include "CmdList.h"
#include "dbgdata.h"
#include "memory.h"

#include <sstream>
#include <set>
#include <fstream>
#include <iomanip>

DEFINE_CMD(free_pool)
{
    size_t size = EXT_F_IntArg(args, 1, 0);
    dump_free_pool(size);
}

DEFINE_CMD(bigpool)
{
    dump_big_pool();
}

DEFINE_CMD(poolrange)
{
    dump_pool_range();
}

DEFINE_CMD(pooltrack)
{
    dump_pool_track();
}

DEFINE_CMD(poolmetrics)
{
    dump_pool_metrics();
}

DEFINE_CMD(tpool)
{
    size_t addr = EXT_F_IntArg(args, 1, 0);
    tpool(addr);
}

DEFINE_CMD(poolhdr)
{
    size_t addr = EXT_F_IntArg(args, 1, 0);
    poolhdr(addr);
}

void dump_free_pool(size_t size)
{
    try
    {
        size -= 0x10;
        size_t paged_pool_des = EXT_F_READ<size_t>(readDbgDataAddr(DEBUG_DATA_ExpPagedPoolDescriptorAddr));
        size_t paged_pool_num = EXT_F_READ<uint32_t>(readDbgDataAddr(DEBUG_DATA_ExpNumberOfPagedPoolsAddr));

        std::stringstream ss;
        for (size_t i = 0; i < paged_pool_num; i++)
        {

            size_t paged_pool_descriptor = paged_pool_des + 0x1140 * i;


            ss << std::hex << std::showbase
                << "\n" << std::string(0x40, '*') << "\n"
                << "Paged Pool Metrics :" << paged_pool_descriptor << "\n"
                << std::setw(20) << "pool index :" << i << "\n\n";


            std::vector<size_t> pages;
            size_t std::count = 0;
            size_t j = std::size / 0x10;
            {

                size_t head = paged_pool_descriptor + 0x140 + 0x10 * j;
                size_t curr = EXT_F_READ<size_t>(head);
                while (curr != head)
                {


                    size_t pool_entry = curr;

                    size_t page = pool_entry & 0xFFFFFFFFFFFFF000;
                    if (std::find(pages.begin(), pages.end(), page) == pages.end() && pool_entry != 0)
                        pages.push_back(page);

                    curr = EXT_F_READ<size_t>(curr);
                    //ss << "\t<link cmd=\"!pool " << hex << showbase << curr << "\">" << curr << "</link>\n";
                    ss << std::hex << std::showbase
                        << "<link cmd=\"dt nt!_POOL_HEADER " << std::setfill('0') << std::setw(16) << pool_entry - 0x10
                        << "\">"
                        << std::setfill('0') << std::setw(16) << pool_entry
                        << "</link>"
                        << "[<link cmd=\"!pool " << std::setfill('0') << std::setw(16) << page
                        << "\">Page " << std::dec << std::noshowbase << std::find(pages.begin(), pages.end(), page) - pages.begin()
                        << "</link>]\t";

                    if (++count % 0x4 == 0 && count != 0)
                        ss << "\n";
                };
                ss << "\n";
            }
            ss << std::endl;

        }

        EXT_F_DML(ss.str().c_str());
    }
    FC;
}


void dump_big_pool()
{
    uint8_t* page_x3_buffer = new uint8_t[0x3000];

    try
    {
        size_t big_pool_addr = EXT_F_READ<size_t>(EXT_F_Sym2Addr("nt!PoolBigPageTable"));
        size_t big_pool_size = EXT_F_READ<size_t>(EXT_F_Sym2Addr("nt!PoolBigPageTableSize"));

        std::stringstream ss;

        size_t item_count = 0;
        size_t page_x3_index = 0;

        for (size_t item_addr = big_pool_addr; item_count < big_pool_size; item_addr += 0x18)
        {
            if ((item_count % 0x200) == 0)
            {
                size_t bytes_read = 0;
                EXT_D_IDebugDataSpaces->ReadVirtual(big_pool_addr + page_x3_index * 0x3000, page_x3_buffer, 0x3000, (PULONG)&bytes_read);

                page_x3_index++;
            }

            item_count++;

            CBigPoolHeader pool_entry(page_x3_buffer + 0x18 * (item_count % 0x200));

            if (!like_kaddr(pool_entry.va))
                continue;

            bool b_free = ((pool_entry.va & 0x1) == 0x1);

            ss.str("");
            ss << std::hex << std::showbase
                << std::setw(8) << item_count << " "
                << std::setw(10) << (b_free ? "Free" : "Allocated") << " "
                << std::setw(18) << pool_entry.va << ", "
                << std::setw(18) << pool_entry.size << " "
                << "[" << pool_entry.tag << "] "
                << std::setw(10) << pool_entry.pool_type << " ";


            if ((pool_entry.pool_type & 0x1) != 0)
                ss << "Paged Pool ";
            else
                ss << "Non-Paged Pool ";

            if ((pool_entry.pool_type & 0x200) != 0)
                ss << "| NX ";
            if ((pool_entry.pool_type & 0x20) != 0)
                ss << "| SessionPool ";
            if ((pool_entry.pool_type & 0x4) != 0)
                ss << "| CacheAligned ";
            if ((pool_entry.pool_type & 0x2) != 0)
                ss << "| MustSucceed ";

            ss << std::endl;

            EXT_F_OUT(ss.str().c_str());

        }
    }
    FC;

    delete[] page_x3_buffer;
}

void dump_pool_track()
{
    try
    {
        size_t pool_track_addr = EXT_F_READ<size_t>(EXT_F_Sym2Addr("nt!PoolTrackTable"));
        size_t pool_track_size = EXT_F_READ<size_t>(EXT_F_Sym2Addr("nt!PoolTrackTableSize"));

        std::stringstream ss;
        for (size_t item_addr = pool_track_addr; item_addr < pool_track_addr + pool_track_size; item_addr += 0x28)
        {
            uint8_t tag[5] = { 0, };
            tag[0] = EXT_F_READ<uint8_t>(item_addr + 0);
            tag[1] = EXT_F_READ<uint8_t>(item_addr + 1);
            tag[2] = EXT_F_READ<uint8_t>(item_addr + 2);
            tag[3] = EXT_F_READ<uint8_t>(item_addr + 3);

            uint32_t np_alloc = EXT_F_READ<uint32_t>(item_addr + 0x04);
            uint32_t np_free = EXT_F_READ<uint32_t>(item_addr + 0x08);
            size_t np_bytes = EXT_F_READ<size_t>(item_addr + 0x10);

            uint32_t p_alloc = EXT_F_READ<uint32_t>(item_addr + 0x18);
            uint32_t p_free = EXT_F_READ<uint32_t>(item_addr + 0x1C);
            size_t p_bytes = EXT_F_READ<size_t>(item_addr + 0x20);

            ss.str("");
            ss << std::hex << std::showbase
                << "[" << tag << "] "
                << std::setw(10) << np_alloc << " A-F "
                << std::setw(10) << np_free << " "
                << std::setw(18) << np_bytes << " bytes, "
                << std::setw(10) << p_alloc << " A-F "
                << std::setw(10) << p_free << " "
                << std::setw(18) << p_bytes << " bytes";

            ss << std::endl;

            EXT_F_OUT(ss.str().c_str());
        }
    }
    FC;
}

void dump_pool_range()
{
    try
    {
        std::vector<POOL_METRICS> pool_metrics;

        size_t vector_pool_addr = EXT_F_Sym2Addr("nt!PoolVector");
        size_t non_paged_pool_addr = EXT_F_READ<size_t>(vector_pool_addr);
        size_t paged_pool_addr = EXT_F_READ<size_t>(vector_pool_addr + 8);

        size_t non_paged_pool_count = EXT_F_READ<uint32_t>(EXT_F_Sym2Addr("nt!ExpNumberOfNonPagedPools"));
        size_t paged_pool_count = EXT_F_READ<uint32_t>(EXT_F_Sym2Addr("nt!ExpNumberOfPagedPools"));


        size_t mi_state_addr = EXT_F_Sym2Addr("nt!MiState");
        ExtRemoteTyped mi_state("(nt!_MI_SYSTEM_INFORMATION*)@$extin", mi_state_addr);


        auto system_node_info = mi_state.Field("Hardware.SystemNodeInformation");
        size_t non_paged_pool_start = system_node_info.Field("NonPagedPoolFirstVa").GetUlong64();

        size_t non_paged_pool_end = non_paged_pool_start;
        for (size_t i = 0; i < 3; i++)
        {
            size_t size_of_bitmap = system_node_info.Field("NonPagedBitMap").ArrayElement(i).Field("SizeOfBitMap").GetUlong64();
            non_paged_pool_end = std::max(non_paged_pool_end, non_paged_pool_start + size_of_bitmap * 0x08);
        }

        auto dyn_bitmap_paged_pool = mi_state.Field("SystemVa.DynamicBitMapPagedPool");
        size_t paged_pool_start = dyn_bitmap_paged_pool.Field("BaseVa").GetUlong64();
        size_t paged_pool_end = paged_pool_start + dyn_bitmap_paged_pool.Field("MaximumSize").GetUlong64() * 0x1000;

        ExtRemoteTypedList pses_list = ExtNtOsInformation::GetKernelProcessList();

        std::set<size_t> session_addrs;


        for (pses_list.StartHead(); pses_list.HasNode(); pses_list.Next())
        {
            ExtRemoteTyped proc_node = pses_list.GetTypedNode();
            size_t proc_addr = pses_list.GetNodeOffset();

            size_t session_addr = proc_node.Field("Session").GetUlong64();

            if (session_addr != 0)
                session_addrs.insert(session_addr);
        }

        for (auto& session_addr : session_addrs)
        {
            ExtRemoteTyped session_space("(nt!_MM_SESSION_SPACE*)@$extin", session_addr);

            auto session_paged_pool = session_space.Field("PagedPool");

            POOL_METRICS session_pool_metrics = { 0, };
            session_pool_metrics._pool_start = session_space.Field("PagedPoolStart").GetUlong64();
            session_pool_metrics._pool_end = session_space.Field("PagedPoolEnd").GetUlong64();
            std::stringstream ss;
            ss << "Session " << session_space.Field("SessionId").GetUlong();
            session_pool_metrics._comment = ss.str();

            size_t pending_frees = session_paged_pool.Field("PendingFrees.Next").GetUlong64();
            size_t pending_free_depth = session_paged_pool.Field("PendingFreeDepth").GetUlong();
            size_t curr = pending_frees;

            size_t session_pool_desc_addr = session_addr + session_space.GetTypeFieldOffset("nt!_MM_SESSION_SPACE", "PagedPool");
            session_pool_metrics._pool_addr = session_pool_desc_addr;

            pool_metrics.push_back(session_pool_metrics);
        }

        for (size_t i = 0; i < non_paged_pool_count; i++)
        {
            ExtRemoteTyped non_paged_pool("(nt!_POOL_DESCRIPTOR*)@$extin", non_paged_pool_addr + i * 0x1140);

            POOL_METRICS non_paged_pool_metrics = { 0, };
            non_paged_pool_metrics._pool_addr = non_paged_pool_addr + i * 0x1140;
            non_paged_pool_metrics._pool_start = non_paged_pool_start;
            non_paged_pool_metrics._pool_end = non_paged_pool_end;
            non_paged_pool_metrics._pool_index = non_paged_pool.Field("PoolIndex").GetUlong();
            non_paged_pool_metrics._pool_type = non_paged_pool.Field("PoolType").GetUlong();
            non_paged_pool_metrics._total_bytes = non_paged_pool.Field("BytesAllocated").GetUlong64();
            non_paged_pool_metrics._total_pages = non_paged_pool.Field("PagesAllocated").GetUlong64();
            non_paged_pool_metrics._total_big_pages = non_paged_pool.Field("BigPagesAllocated").GetUlong64();
            std::stringstream ss;
            ss << "Non-Paged Pool " << i;
            non_paged_pool_metrics._comment = ss.str();

            size_t pending_frees = non_paged_pool.Field("PendingFrees.Next").GetUlong64();
            size_t pending_free_depth = non_paged_pool.Field("PendingFreeDepth").GetUlong();
            size_t curr = pending_frees;

            pool_metrics.push_back(non_paged_pool_metrics);
        }



        for (size_t i = 0; i < paged_pool_count; i++)
        {
            ExtRemoteTyped paged_pool("(nt!_POOL_DESCRIPTOR*)@$extin", paged_pool_addr + i * 0x1140);

            POOL_METRICS paged_pool_metrics = { 0, };
            paged_pool_metrics._pool_addr = paged_pool_addr + i * 0x1140;
            paged_pool_metrics._pool_start = paged_pool_start;
            paged_pool_metrics._pool_end = paged_pool_end;
            paged_pool_metrics._pool_index = paged_pool.Field("PoolIndex").GetUlong();
            paged_pool_metrics._pool_type = paged_pool.Field("PoolType").GetUlong();
            paged_pool_metrics._total_bytes = paged_pool.Field("BytesAllocated").GetUlong64();
            paged_pool_metrics._total_pages = paged_pool.Field("PagesAllocated").GetUlong64();
            paged_pool_metrics._total_big_pages = paged_pool.Field("BigPagesAllocated").GetUlong64();
            std::stringstream ss;
            ss << "Paged Pool " << i;
            paged_pool_metrics._comment = ss.str();

            size_t pending_frees = paged_pool.Field("PendingFrees.Next").GetUlong64();
            size_t pending_free_depth = paged_pool.Field("PendingFreeDepth").GetUlong();
            size_t curr = pending_frees;

            pool_metrics.push_back(paged_pool_metrics);
        }




        std::ofstream ss(R"(E:\pool_range.txt)", std::ios::out | std::ios::trunc);
        for (auto& pool_info : pool_metrics)
        {
            ss << std::hex << std::showbase
                << "\n" << std::string(0x40, '*') << "\n"
                << std::setw(20) << "Paged Pool Metrics :" << pool_info._pool_addr << "\n"
                << std::setw(20) << "comment :" << pool_info._comment << "\n"
                << std::setw(20) << "pool index :" << pool_info._pool_index << "\n"
                << std::setw(20) << "total bytes :" << pool_info._total_bytes << "(" << pool_info._total_bytes / 1024 / 1024 << " MB)\n"
                << std::setw(20) << "total pages :" << pool_info._total_pages << "\n"
                << std::setw(20) << "range start :" << pool_info._pool_start << "\n"
                << std::setw(20) << "range end :" << pool_info._pool_end << "\n"
                << std::setw(20) << "total big pages :" << pool_info._total_big_pages << "\n\n";

            ss << std::setw(20) << "pending frees :" << "\n";
            for (auto& pending : pool_info._pending_frees)
            {
                ss << "\t" << pending;
            }
            ss << std::endl;

            ss << std::setw(20) << "free lists :" << "\n";

            for (auto& free_list : pool_info._free_lists)
            {
                ss << "[" << free_list.first * 0x10 << "]\n";
                for (auto& item : free_list.second)
                {
                    ss << "\t" << item;
                }
                ss << "\n";
            }
            ss << std::endl;
        }
    }
    FC;
}


void dump_pool_metrics()
{
    try
    {
        size_t exp_pool_flags_addr = EXT_F_Sym2Addr("nt!ExpPoolFlags");
        uint32_t exp_pool_flags = EXT_F_READ<uint32_t>(exp_pool_flags_addr);
        EXT_F_OUT("nt!ExpPoolFlags: 0x%08x\n", exp_pool_flags);

        size_t exp_session_pool_la_addr = EXT_F_Sym2Addr("nt!ExpSessionPoolLookaside");
        size_t exp_session_pool_la = EXT_F_READ<size_t>(exp_session_pool_la_addr);
        EXT_F_OUT("nt!ExpSessionPoolLookaside: 0x%I64x\n", exp_session_pool_la_addr);

        uint32_t exp_session_pool_small_lists = EXT_F_READ<uint32_t>(EXT_F_Sym2Addr("nt!ExpSessionPoolSmallLists"));
        EXT_F_OUT("nt!ExpSessionPoolSmallLists: 0x%08x\n", exp_session_pool_small_lists);

        uint32_t exp_number_of_paged_pools = EXT_F_READ<uint32_t>(EXT_F_Sym2Addr("nt!ExpNumberOfPagedPools"));
        EXT_F_OUT("nt!ExpNumberOfPagedPools: 0x%08x\n", exp_number_of_paged_pools);

        uint32_t exp_number_of_non_paged_pools = EXT_F_READ<uint32_t>(EXT_F_Sym2Addr("nt!ExpNumberOfNonPagedPools"));
        EXT_F_OUT("nt!ExpNumberOfNonPagedPools: 0x%08x\n", exp_number_of_non_paged_pools);

        size_t exp_paged_pool_descriptor = EXT_F_READ<size_t>(EXT_F_Sym2Addr("nt!ExpPagedPoolDescriptor"));
        EXT_F_OUT("nt!ExpPagedPoolDescriptor: 0x%I64x\n", EXT_F_Sym2Addr("nt!ExpPagedPoolDescriptor"));

        size_t exp_non_paged_pool_descriptor = EXT_F_READ<size_t>(EXT_F_Sym2Addr("nt!ExpNonPagedPoolDescriptor"));
        EXT_F_OUT("nt!ExpNonPagedPoolDescriptor: 0x%I64x\n", EXT_F_Sym2Addr("nt!ExpNonPagedPoolDescriptor"));


    }
    FC;
}

void tpool(size_t addr)
{
    try
    {
        for (size_t i = 0; i < 0x08; i++)
        {
            size_t item_addr = addr + 0x10 + 0x450 * i;
            EXT_F_OUT("[%d] token at 0x%I64x\n", i, item_addr);
            EXT_F_OUT("\tnext : 0x%I64x\n", EXT_F_READ<size_t>(item_addr));
            EXT_F_OUT("\tmodel and size : 0x%08x 0x%08x\n\n", EXT_F_READ<uint16_t>(item_addr + 0x10), EXT_F_READ<uint16_t>(item_addr + 0x14));
        }
    }
    FC;
}

void poolhdr(size_t addr)
{
    try
    {
        uint8_t prev_size = EXT_F_READ<uint8_t>(addr);
        uint8_t pool_index = EXT_F_READ<uint8_t>(addr + 1);
        uint8_t block_size = EXT_F_READ<uint8_t>(addr + 2);
        uint8_t pool_type = EXT_F_READ<uint8_t>(addr + 3);

        char tag[5] = { 0, };
        tag[0] = EXT_F_READ<uint8_t>(addr + 4);
        tag[1] = EXT_F_READ<uint8_t>(addr + 5);
        tag[2] = EXT_F_READ<uint8_t>(addr + 6);
        tag[3] = EXT_F_READ<uint8_t>(addr + 7);

        uint16_t allocator_bt_index = EXT_F_READ<uint16_t>(addr + 0x08);
        uint16_t pool_tag_hash = EXT_F_READ<uint16_t>(addr + 0x0A);

        size_t process_billed = EXT_F_READ<size_t>(addr + 0x08);

        EXT_F_OUT("%30s : 0x%02x\n", "Prev Size", (uint16_t)prev_size);
        EXT_F_OUT("%30s : 0x%02x\n", "Pool Index", (uint16_t)pool_index);
        EXT_F_OUT("%30s : 0x%02x\n", "Block Size", (uint16_t)block_size);
        EXT_F_OUT("%30s : 0x%02x\n", "Pool Type", (uint16_t)pool_type);
        EXT_F_OUT("%30s : 0x%s\n", "Tag", tag);
        EXT_F_OUT("%30s : 0x%04x\n", "Allocator BT Index", allocator_bt_index);
        EXT_F_OUT("%30s : 0x%04x\n", "Pool Tag Hash", pool_tag_hash);
        EXT_F_OUT("%30s : 0x%I64x\n", "Process Billed", process_billed);

        if ((std::string)tag == "Free")
        {
            size_t prev_free = EXT_F_READ<size_t>(addr + 0x10);
            size_t next_free = EXT_F_READ<size_t>(addr + 0x18);

            EXT_F_OUT("%30s : 0x%I64x\n", "Prev Free", prev_free);
            EXT_F_OUT("%30s : 0x%I64x\n", "Next Free", next_free);
        }
    }
    FC;
}


