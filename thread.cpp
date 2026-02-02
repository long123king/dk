#include "thread.h"
#include "CmdExt.h"
#include "CmdList.h"

#include <iomanip>

DEFINE_CMD(threads)
{
    size_t proc_addr = EXT_F_IntArg(args, 1, 0);

    if (proc_addr == 0)
    {
        EXT_F_ERR("Usage: !dk threads <proc_addr>\n");
        EXT_F_ERR("proc_addr can't be 0 or empty\n");
        return;
    }

    dump_process_threads(proc_addr);
}

void dump_process_threads(size_t process_addr)
{
    try
    {
        ExtRemoteTyped proc("(nt!_EPROCESS*)@$extin", process_addr);
        size_t thread_list_head_addr = process_addr + proc.GetFieldOffset("ThreadListHead");

        ExtRemoteTypedList threads_list(thread_list_head_addr, "nt!_ETHREAD", "ThreadListEntry");
        std::stringstream ss;
        for (threads_list.StartHead(); threads_list.HasNode(); threads_list.Next())
        {
            auto thread = threads_list.GetTypedNode();
            size_t thread_addr = threads_list.GetNodeOffset();

            size_t unique_process = thread.Field("Cid.UniqueProcess").GetUlongPtr();
            size_t unique_thread = thread.Field("Cid.UniqueThread").GetUlongPtr();  
            size_t teb_addr = thread.Field("Tcb.Teb").GetUlongPtr();

            auto thread_token_info = get_thread_token(thread_addr);

            size_t thread_token_addr = get<0>(thread_token_info);

            size_t thread_silo = thread.Field("Silo").GetUlongPtr();

            size_t start_addr = thread.Field("Win32StartAddress").GetUlongPtr();

            std::string start_func_name = get<0>(EXT_F_Addr2Sym(start_addr));


            ss << "\tThread: <link cmd=\"!thread " << std::hex << std::showbase << thread_addr << "\">" << thread_addr << "</link> "
                << "<link cmd=\"dt nt!_ETHREAD " << thread_addr << "\">dt</link> "
                << "<link cmd=\"!dk obj " << thread_addr - 0x30 << "\">detail</link> "
                << "<link cmd=\".thread " << thread_addr << "; kf;\">switch</link>    "
                << "Cid: " << std::setw(6) << unique_process << "." << std::left << std::setw(6) << unique_thread << std::right << "    Thread Func: ";


            if (start_func_name.empty())
                ss << "<link cmd=\"u " << start_addr << "\">" << start_addr << "</link>";
            else
                ss << start_func_name;


            if (thread_token_addr != 0)
            {
                ss << "\t\tImpersonation Token: <link cmd=\"!token " << thread_token_addr << "\">" << thread_token_addr << "</link> "
                    << "<link cmd=\"!dk obj " << thread_token_addr - 0x30 << "\">detail</link> "
                    << "<link cmd=\"dt nt!_TOKEN " << thread_token_addr << "\">dt</link> "
                    << getImpersonationLevel(get<1>(thread_token_info)) << "(" << get<1>(thread_token_info) << ") ";
            }

            if (thread_silo != 0xfffffffffffffffd)
            {
                ss << " silo: " << thread_silo;
            }
            else
            {
                ss << " silo: Inherited";
            }

            ss << std::endl;
        }

        EXT_F_DML(ss.str().c_str());
    }
    FC;
}

std::tuple<size_t, size_t> get_thread_token(size_t thread_addr)
{
    size_t token = 0;
    size_t level = 0;
    try
    {
        ExtRemoteTyped thread("(nt!_ETHREAD*)@$extin", thread_addr);
        bool impersonating = (thread.Field("CrossThreadFlags").GetUlong() & 8) != 0;
        if (impersonating)
        {
            token = thread.Field("ClientSecurity.ImpersonationToken").GetUlongPtr() & 0xFFFFFFFFFFFFFFF8;
            level = thread.Field("ClientSecurity.ImpersonationLevel").GetUlong64();
        }
    }
    FC;
    return std::make_tuple(token, level);
}

std::string
getImpersonationLevel(
    __in const size_t level
)
{
    static std::map<size_t, const char*> s_impersonation_level_map{ {
        { SecurityAnonymous, "SecurityAnonymous" },
        { SecurityIdentification, "SecurityIdentification" },
        { SecurityImpersonation, "SecurityImpersonation" },
        { SecurityDelegation, "SecurityDelegation" },
    } };

    auto it = s_impersonation_level_map.find(level);

    if (it != s_impersonation_level_map.end())
        return it->second;

    return "";
}