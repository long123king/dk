#include "dbgdata.h"
#include "process.h"
#include "token.h"
#include "object.h"
#include "type.h"
#include "module.h"
#include "dml.h"
#include "thread.h"
#include "memory.h"
#include "handle.h"
#include "page.h"
#include "pool.h"
#include "session.h"
#include "list.h"
#include "stack.h"
#include "pe.h"
#include "clfs.h"
#include "ttd_cmd.h"
#include "usermode.h"
#include "heap.h"

#include "model.h"

#include "CmdList.h"
#include "CmdExt.h" 
#include <engextcpp.hpp>
#include <iostream>
#include <iomanip>

void cmd_help(std::vector<std::string>& args)
{
    if (args.size() > 1)
    {
         CMD_LIST->PrintUsage(args[1]);
    }
    else
    {
        CMD_LIST->PrintAllHelp();
    }
}

void RegisterCmdHandlers()
{
    CMD_LIST->RegisterCmdHandler("help", 
        CMD_HANDLER(help), 
        "Displays help for commands", 
        "!dk help [command]");

    CMD_LIST->RegisterCmdHandler("dbgdata",         
        CMD_HANDLER(dbgdata), 
        "Dumps Debugger Data Block", 
        "!dk dbgdata");

    CMD_LIST->RegisterCmdHandler("pses",
        CMD_HANDLER(pses),
        "Lists all processes",
        "!dk pses");

    CMD_LIST->RegisterCmdHandler("kill",
        CMD_HANDLER(kill),
        "Terminates a process",
        "!dk kill <EPROCESS_Addr>");

    CMD_LIST->RegisterCmdHandler("process",
        CMD_HANDLER(process),
        "Dump detailed information about a process",
        "!dk process [EPROCESS_Addr]");

    CMD_LIST->RegisterCmdHandler("ps_flags",
        CMD_HANDLER(ps_flags),
        "Analyzes process flags",
        "!dk ps_flags <EPROCESS_Addr>");

    CMD_LIST->RegisterCmdHandler("sid",
        CMD_HANDLER(sid),
        "Dump a Security Identifier (SID)",
        "!dk sid <SID_Addr>");

    CMD_LIST->RegisterCmdHandler("token",
        CMD_HANDLER(token),
        "Dump a Security Token object",
        "!dk token [Token_Addr]");

    CMD_LIST->RegisterCmdHandler("add_privilege",
        CMD_HANDLER(add_privilege),
        "Adds a privilege to current token",
        "!dk add_privilege <Priv_Value>");

    CMD_LIST->RegisterCmdHandler("obj",
        CMD_HANDLER(obj),
        "Displays Object Header and type info",
        "!dk obj <Object_Addr>");

    CMD_LIST->RegisterCmdHandler("gobj",
        CMD_HANDLER(gobj),
        "Dumps Global Root Object Directory",
        "!dk gobj");

    CMD_LIST->RegisterCmdHandler("obj_dir",
        CMD_HANDLER(obj_dir),
        "Lists Object Directory contents",
        "!dk obj_dir <Directory_Addr>");

    CMD_LIST->RegisterCmdHandler("types",
        CMD_HANDLER(types),
        "Lists managed type information",
        "!dk types");

    CMD_LIST->RegisterCmdHandler("lmu",
        CMD_HANDLER(lmu),
        "List user modules",
        "!dk lmu");

    CMD_LIST->RegisterCmdHandler("lmk",
        CMD_HANDLER(lmk),
        "List kernel modules",
        "!dk lmk");

    CMD_LIST->RegisterCmdHandler("lm",
        CMD_HANDLER(lm),
        "List modules",
        "!dk lm");

    CMD_LIST->RegisterCmdHandler("load_dml",
        CMD_HANDLER(dml),
        "Loads a DML file",
        "!dk load_dml <File_Path>");

    CMD_LIST->RegisterCmdHandler("threads",
        CMD_HANDLER(threads),
        "Lists threads for the current process",
        "!dk threads");

    CMD_LIST->RegisterCmdHandler("size",
        CMD_HANDLER(size),
        "Show size in different format",
        "!dk size <Size_Value>");

    CMD_LIST->RegisterCmdHandler("va_regions",
        CMD_HANDLER(va_regions),
        "Analyzes virtual address regions",
        "!dk va_regions");

    CMD_LIST->RegisterCmdHandler("page",
        CMD_HANDLER(page),
        "Dumps page table translation",
        "!dk page <Virtual_Addr>");

    CMD_LIST->RegisterCmdHandler("pages",
        CMD_HANDLER(pages),
        "Dumps page table entries for region",
        "!dk pages <Virtual_Addr>");

    CMD_LIST->RegisterCmdHandler("hole",
        CMD_HANDLER(hole),
        "Inspects a memory hole",
        "!dk hole <Address>");

    CMD_LIST->RegisterCmdHandler("vad",
        CMD_HANDLER(vad),
        "Displays VAD information",
        "!dk vad <MMVAD_Addr>");

    CMD_LIST->RegisterCmdHandler("regs",
        CMD_HANDLER(regs),
        "Displays registers",
        "!dk regs");

    CMD_LIST->RegisterCmdHandler("as_qword",
        CMD_HANDLER(as_qword),
        "Displays memory as qwords, with heuristic type enrichment.",
        "!dk as_qword <Address> [Count]");

    CMD_LIST->RegisterCmdHandler("tpool",
        CMD_HANDLER(tpool),
        "Displays TPool Entries (reserved).",
        "!dk tpool");

    CMD_LIST->RegisterCmdHandler("poolhdr",
        CMD_HANDLER(poolhdr),
        "Decodes pool header",
        "!dk poolhdr <Address>");

    CMD_LIST->RegisterCmdHandler("pool",
        CMD_HANDLER(as_qword),
        "Displays memory as qwords (pool alias)",
        "!dk pool <Address>");

    CMD_LIST->RegisterCmdHandler("bigpool",
        CMD_HANDLER(bigpool),
        "Dumps kernel Big Pool allocations",
        "!dk bigpool");

    CMD_LIST->RegisterCmdHandler("poolrange",
        CMD_HANDLER(poolrange),
        "Dumps pool allocation range",
        "!dk poolrange");

    CMD_LIST->RegisterCmdHandler("pooltrack",
        CMD_HANDLER(pooltrack),
        "Analyzes pool tracking info",
        "!dk pooltrack");

    CMD_LIST->RegisterCmdHandler("poolmetrics",
        CMD_HANDLER(poolmetrics),
        "Displays pool metrics",
        "!dk poolmetrics");

    CMD_LIST->RegisterCmdHandler("free_pool",
        CMD_HANDLER(free_pool),
        "Searches for free pool chunks",
        "!dk free_pool <Size_Hex>");

    CMD_LIST->RegisterCmdHandler("as_mem",
        CMD_HANDLER(as_mem),
        "Displays memory content, with heuristic type enrichment.",
        "!dk as_mem <Address>");

    CMD_LIST->RegisterCmdHandler("ex_mem",
        CMD_HANDLER(ex_mem),
        "Examines memory",
        "!dk ex_mem <Address>");
		
    CMD_LIST->RegisterCmdHandler("carve_strs",
        CMD_HANDLER(carve_strs),
        "Carve ansi strings in memory",
        "!dk carve_strs <Addr> <Len>");		
		
	CMD_LIST->RegisterCmdHandler("carve_ustrs",
        CMD_HANDLER(carve_ustrs),
        "Carve Unicode strings in memory",
        "!dk carve_ustrs <Addr> <Len>");	
		
	CMD_LIST->RegisterCmdHandler("pages_2_svg",
        CMD_HANDLER(pages_2_svg),
        "Exports continuous pages content to SVG",
        "!dk pages_2_svg <Addr> <Count> <Svg_Filename>");

    CMD_LIST->RegisterCmdHandler("page_2_svg",
        CMD_HANDLER(page_2_svg),
        "Exports page content to SVG",
        "!dk page_2_svg <Addr> <Svg_Filename>");

    CMD_LIST->RegisterCmdHandler("sessions",
        CMD_HANDLER(sessions),
        "Lists sessions",
        "!dk sessions");

    CMD_LIST->RegisterCmdHandler("sessionpool",
        CMD_HANDLER(sessionpool),
        "Displays session pool",
        "!dk sessionpool");

    CMD_LIST->RegisterCmdHandler("args",
        CMD_HANDLER(args),
        "Argument analysis",
        "!dk args");

    CMD_LIST->RegisterCmdHandler("handle_table",
        CMD_HANDLER(handle_table),
        "Displays handle table",
        "!dk handle_table <TableAddr>");

    CMD_LIST->RegisterCmdHandler("khandles",
        CMD_HANDLER(khandles),
        "Kernel handles",
        "!dk khandles");

    CMD_LIST->RegisterCmdHandler("link",
        CMD_HANDLER(link),
        "Follows list link",
        "!dk link <Address>");

    CMD_LIST->RegisterCmdHandler("flink",
        CMD_HANDLER(flink),
        "Follows forward link",
        "!dk flink <Address>");

    CMD_LIST->RegisterCmdHandler("kall",
        CMD_HANDLER(kall),
        "Dumps all stack backtrace information, for single process",
        "!dk kall");

    CMD_LIST->RegisterCmdHandler("pkall",
        CMD_HANDLER(pkall),
        "Dumps all stack backtrace information, for all process",
        "!dk pkall");

    CMD_LIST->RegisterCmdHandler("memcpy",
        CMD_HANDLER(memcpy),
        "Copies memory",
        "!dk memcpy <Dest> <Src> <Size>");

    CMD_LIST->RegisterCmdHandler("peguid",
        CMD_HANDLER(peguid),
        "Extracts PE GUID",
        "!dk peguid <Address>");

    CMD_LIST->RegisterCmdHandler("ls_model",
        CMD_HANDLER(ls_model),
        "List model objects, Available Model Objects:\n\t - Sessions\n\t - Settings\n\t - State\n\t - Utility\n\t - LastEvent\n",
        "!dk ls_model <root_model_object>");

    CMD_LIST->RegisterCmdHandler("ls_sessions",
        CMD_HANDLER(ls_sessions),
        "Lists model sessions",
        "!dk ls_sessions");

    CMD_LIST->RegisterCmdHandler("ls_processes",
        CMD_HANDLER(ls_processes),
        "Lists model processes",
        "!dk ls_processes <session_id>");

    CMD_LIST->RegisterCmdHandler("ls_threads",
        CMD_HANDLER(ls_threads),
        "Lists model threads",
        "!dk ls_threads <session_id> <process_id>");

    CMD_LIST->RegisterCmdHandler("ls_modules",
        CMD_HANDLER(ls_modules),
        "Lists model modules",
        "!dk ls_modules <session_id> <process_id>");

    CMD_LIST->RegisterCmdHandler("ls_handles",
        CMD_HANDLER(ls_handles),
        "Lists model handles",
        "!dk ls_handles <session_id> <process_id>");

    CMD_LIST->RegisterCmdHandler("ps_ttd",
        CMD_HANDLER(ps_ttd),
        "Process TTD info",
        "!dk ps_ttd");

    CMD_LIST->RegisterCmdHandler("session_ttd",
        CMD_HANDLER(session_ttd),
        "Session TTD info",
        "!dk session_ttd");

    CMD_LIST->RegisterCmdHandler("ttd_calls",
        CMD_HANDLER(ttd_calls),
        "List TTD calls",
        "!dk ttd_calls");
		
	CMD_LIST->RegisterCmdHandler("ttd_vis_info",
        CMD_HANDLER(ttd_vis_info),
        "Generate TTD Visualization Information",
        "!dk ttd_vis_info");

    CMD_LIST->RegisterCmdHandler("ttd_mem_access",
        CMD_HANDLER(ttd_mem_access),
        "TTD memory access analysis",
        "!dk ttd_mem_access");

    CMD_LIST->RegisterCmdHandler("ttd_mem_use",
        CMD_HANDLER(ttd_mem_use),
        "TTD memory usage info",
        "!dk ttd_mem_use");

    CMD_LIST->RegisterCmdHandler("cur_context",
        CMD_HANDLER(cur_context),
        "Displays the current context",
        "!dk cur_context");

    CMD_LIST->RegisterCmdHandler("exec",
        CMD_HANDLER(exec),
        "Execute command",
        "!dk exec <Cmd>");

    CMD_LIST->RegisterCmdHandler("mobj",
        CMD_HANDLER(mobj),
        "Dump model object under path.",
        "!dk mobj <mobj_path>");

    CMD_LIST->RegisterCmdHandler("mobj_at",
        CMD_HANDLER(mobj_at),
        "Dump model object under path, with specific index.",
        "!dk mobj_at <mobj_path> <idx>");

    CMD_LIST->RegisterCmdHandler("call",
        CMD_HANDLER(call),
        "Call function",
        "!dk call <mobj_path>");

    CMD_LIST->RegisterCmdHandler("ccall",
        CMD_HANDLER(ccall),
        "Custom call",
        "!dk ccall <mobj_path> <context_path>");

    CMD_LIST->RegisterCmdHandler("ldttd",
        CMD_HANDLER(ldttd),
        "Loads/Initializes TTD support",
        "!dk ldttd");
		
	CMD_LIST->RegisterCmdHandler("ldttd_by_range",
        CMD_HANDLER(ldttd),
        "Loads/Initializes TTD support",
        "!dk ldttd_by_range <Start> <Stop>");

    CMD_LIST->RegisterCmdHandler("dump_ttd_events",
        CMD_HANDLER(dump_ttd_events),
        "Dumps recorded TTD events",
        "!dk dump_ttd_events");
		
	CMD_LIST->RegisterCmdHandler("dump_ttd_events_to_csv",
        CMD_HANDLER(dump_ttd_events_to_csv),
        "Dumps recorded TTD events to CSV file.",
        "!dk dump_ttd_events_to_csv <Filename>");

    CMD_LIST->RegisterCmdHandler("usearch_astr",
        CMD_HANDLER(usearch_astr),
        "Search user memory for ASCII string",
        "!dk usearch_astr <String>");

    CMD_LIST->RegisterCmdHandler("usearch_ustr",
        CMD_HANDLER(usearch_ustr),
        "Search user memory for Unicode string",
        "!dk usearch_ustr <String>");

    CMD_LIST->RegisterCmdHandler("usearch_bytes",
        CMD_HANDLER(usearch_bytes),
        "Search user memory for bytes",
        "!dk usearch_bytes <Bytes>");
		
	CMD_LIST->RegisterCmdHandler("usearch_addr_mask",
        CMD_HANDLER(usearch_addr_mask),
        "Search user memory for bytes with Mask (range search)",
        "!dk usearch_addr_mask <Bytes> <Mask>");

    CMD_LIST->RegisterCmdHandler("usearch_addr",
        CMD_HANDLER(usearch_addr),
        "Search user memory for address",
        "!dk usearch_addr <Address>");

    CMD_LIST->RegisterCmdHandler("uaddr_ref_disp",
        CMD_HANDLER(uaddr_ref_disp),
        "User address reference display",
        "!dk uaddr_ref_disp <Address>");

    CMD_LIST->RegisterCmdHandler("uaddr_ref_by",
        CMD_HANDLER(uaddr_ref_by),
        "Find who references user address",
        "!dk uaddr_ref_by <Address>");

    CMD_LIST->RegisterCmdHandler("uaddr_ref_tree",
        CMD_HANDLER(uaddr_ref_tree),
        "User address reference tree",
        "!dk uaddr_ref_tree <Address>");

    CMD_LIST->RegisterCmdHandler("uaddr_analyze2",
        CMD_HANDLER(uaddr_analyze2),
        "Deep user address analysis",
        "!dk uaddr_analyze2 <Address>");

    CMD_LIST->RegisterCmdHandler("uaddr_analyze",
        CMD_HANDLER(uaddr_analyze),
        "Analyze user address",
        "!dk uaddr_analyze <Address>");

    CMD_LIST->RegisterCmdHandler("uaddr_analyze_svg",
        CMD_HANDLER(uaddr_analyze_svg),
        "Analyze user address to SVG",
        "!dk uaddr_analyze_svg <Address>");

    CMD_LIST->RegisterCmdHandler("ustacks",
        CMD_HANDLER(ustacks),
        "Summarizes user-mode stacks",
        "!dk ustacks");

    CMD_LIST->RegisterCmdHandler("uregs",
        CMD_HANDLER(uregs),
        "Displays user-mode registers",
        "!dk uregs");
		
	CMD_LIST->RegisterCmdHandler("unloaded_pe",
        CMD_HANDLER(unloaded_pe),
        "Search potential unloaded PE module",
        "!dk unloaded_pe");

    CMD_LIST->RegisterCmdHandler("heap_ssum",
        CMD_HANDLER(heap_ssum),
        "Displays heap size summary (User Mode)",
        "!dk heap_ssum");

    CMD_LIST->RegisterCmdHandler("heap_bysize",
        CMD_HANDLER(heap_bysize),
        "Lists heap allocations by size (User Mode)",
        "!dk heap_bysize <Size_Hex>");

    CMD_LIST->RegisterCmdHandler("heap_oversize",
        CMD_HANDLER(heap_oversize),
        "Lists heap allocations larger than size (User Mode)",
        "!dk heap_oversize <Size_Hex>");
		
	CMD_LIST->RegisterCmdHandler("pe_hdr",
        CMD_HANDLER(pe_hdr),
        "Dump PE Header",
        "!dk pe_hdr <Addr>");
}

BOOL __stdcall CmdListMain(HANDLE Instance, ULONG Reason, PVOID Reserved)
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:          
        RegisterCmdHandlers();
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void CCmdList::PrintUsage(std::string cmd_name)
{
    auto it = m_cmd_handlers.find(cmd_name);
    if (it != m_cmd_handlers.end())
    {
        if (!it->second.usage.empty())
        {
            EXT_F_OUT("Usage: %s\n", it->second.usage.c_str());
            EXT_F_OUT("Description: %s\n", it->second.description.c_str());
        }
        else
        {
            EXT_F_OUT("No usage information available for command '%s'.\n", cmd_name.c_str());
        }
    }
    else
    {
        EXT_F_OUT("Command '%s' not found.\n", cmd_name.c_str());
    }
}

void CCmdList::PrintAllHelp()
{
    EXT_F_OUT("Supported Commands:\n");

    // 计算列宽（基于现有数据，并设置合理上限）
    size_t maxName = std::string("Command").length();
    size_t maxUsage = std::string("Usage").length();
    for (const auto& pair : m_cmd_handlers)
    {
        maxName = max(maxName, pair.first.length());
        maxUsage = max(maxUsage, pair.second.usage.length());
    }

    // 限制列宽以避免过宽输出
    const size_t kMaxNameWidth = 30;
    const size_t kMaxUsageWidth = 40;
    const size_t kDescWidth = 80; // 描述最多显示这么多字符，超出截断

    size_t nameWidth = min(maxName, kMaxNameWidth);
    size_t usageWidth = min(maxUsage, kMaxUsageWidth);

    auto truncate = [](const std::string& s, size_t w) -> std::string {
        if (s.length() <= w) return s;
        if (w <= 3) return s.substr(0, w);
        return s.substr(0, w - 3) + "...";
    };

    auto pad_right = [&](const std::string& s, size_t w) -> std::string {
        std::string t = truncate(s, w);
        if (t.length() < w)
            t.append(w - t.length(), ' ');
        return t;
    };

    // 打印表头
    std::string header = pad_right("Command", nameWidth) + "  " + pad_right("Usage", usageWidth) + "  " + "Description";
    EXT_F_OUT("%s\n", header.c_str());

    // 分隔线
    std::string sep;
    sep.append(nameWidth, '-');
    sep += "  ";
    sep.append(usageWidth, '-');
    sep += "  ";
    sep.append(kDescWidth, '-');
    EXT_F_OUT("%s\n", sep.c_str());

    // 每条命令一行
    for (const auto& pair : m_cmd_handlers)
    {
        std::string name = pair.first;
        std::string usage = pair.second.usage.empty() ? "" : pair.second.usage;
        std::string desc = pair.second.description.empty() ? "No description available" : pair.second.description;

        std::string row = pad_right(name, nameWidth) + "  " + pad_right(usage, usageWidth) + "  " + truncate(desc, kDescWidth);
        EXT_F_OUT("%s\n", row.c_str());
    }

    EXT_F_OUT("\nType '!dk help <command>' for detailed usage.\n");
}

std::vector<std::string> CCmdList::GetValidCommands()
{
    std::vector<std::string> valid_cmds;

    for (auto cmd : m_cmd_handlers)
        valid_cmds.push_back(cmd.first);

    return valid_cmds;
}
