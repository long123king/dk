#include "stack.h"
#include "CmdExt.h"
#include "CmdList.h"
#include "process.h"

DEFINE_CMD(kall)
{
    dump_threads_stack(curr_proc());
}

DEFINE_CMD(pkall)
{
    dump_all_threads_stack();
}

void dump_threads_stack(size_t process_addr)
{
    try
    {
        //size_t process_addr = curr_proc();
        ExtRemoteTyped proc("(nt!_EPROCESS*)@$extin", process_addr);
        size_t thread_list_head_addr = process_addr + proc.GetFieldOffset("ThreadListHead");

        ExtRemoteTypedList threads_list(thread_list_head_addr, "nt!_ETHREAD", "ThreadListEntry");
        for (threads_list.StartHead(); threads_list.HasNode(); threads_list.Next())
        {
            size_t thread_addr = threads_list.GetNodeOffset();

            EXT_F_OUT("-------------------------------\n");

            size_t trap_frame_addr = threads_list.GetTypedNode().Field("Tcb.TrapFrame").GetUlongPtr();

            stringstream cmd;
            cmd << ".thread " << hex << showbase << thread_addr << ";";

            if (S_OK == EXT_F_EXEC(cmd.str()) && (trap_frame_addr == 0 || EXT_F_ValidAddr(trap_frame_addr)))
                EXT_F_EXEC("kf");
        }
    }
    FC;
}

void dump_all_threads_stack()
{
    try
    {
        ExtRemoteTypedList pses_list = ExtNtOsInformation::GetKernelProcessList();

        for (pses_list.StartHead(); pses_list.HasNode(); pses_list.Next())
        {
            size_t proc_addr = pses_list.GetNodeOffset();
            EXT_F_OUT("-----------[0x%I64x]-----------\n", proc_addr);
            dump_threads_stack(proc_addr);
        }
    }FC;
}
