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

#include "model.h"

#include "CmdList.h"
#include <engextcpp.hpp>

void RegisterCmdHandlers()
{
    CMD_LIST->RegisterCmdHandler("dbgdata",         CMD_HANDLER(dbgdata));
    CMD_LIST->RegisterCmdHandler("pses",            CMD_HANDLER(pses));
    CMD_LIST->RegisterCmdHandler("kill",            CMD_HANDLER(kill));
    CMD_LIST->RegisterCmdHandler("process",         CMD_HANDLER(process));
    CMD_LIST->RegisterCmdHandler("ps_flags",        CMD_HANDLER(ps_flags));
    CMD_LIST->RegisterCmdHandler("sid",             CMD_HANDLER(sid));
    CMD_LIST->RegisterCmdHandler("token",           CMD_HANDLER(token));
    CMD_LIST->RegisterCmdHandler("add_privilege",   CMD_HANDLER(add_privilege));
    CMD_LIST->RegisterCmdHandler("obj",             CMD_HANDLER(obj));
    CMD_LIST->RegisterCmdHandler("gobj",            CMD_HANDLER(gobj));
    CMD_LIST->RegisterCmdHandler("obj_dir",         CMD_HANDLER(obj_dir));
    CMD_LIST->RegisterCmdHandler("types",           CMD_HANDLER(types));
    CMD_LIST->RegisterCmdHandler("lmu",             CMD_HANDLER(lmu));
    CMD_LIST->RegisterCmdHandler("lmk",             CMD_HANDLER(lmk));
    CMD_LIST->RegisterCmdHandler("lm",              CMD_HANDLER(lm));
    CMD_LIST->RegisterCmdHandler("load_dml",        CMD_HANDLER(dml));
    CMD_LIST->RegisterCmdHandler("threads",         CMD_HANDLER(threads));
    CMD_LIST->RegisterCmdHandler("size",            CMD_HANDLER(size));
    CMD_LIST->RegisterCmdHandler("va_regions",      CMD_HANDLER(va_regions));
    CMD_LIST->RegisterCmdHandler("page",            CMD_HANDLER(page));
    CMD_LIST->RegisterCmdHandler("pages",           CMD_HANDLER(pages));
    CMD_LIST->RegisterCmdHandler("hole",            CMD_HANDLER(hole));
    CMD_LIST->RegisterCmdHandler("vad",             CMD_HANDLER(vad));
    CMD_LIST->RegisterCmdHandler("regs",            CMD_HANDLER(regs));
    CMD_LIST->RegisterCmdHandler("as_qword",        CMD_HANDLER(as_qword));
    CMD_LIST->RegisterCmdHandler("tpool",           CMD_HANDLER(tpool));
    CMD_LIST->RegisterCmdHandler("poolhdr",         CMD_HANDLER(poolhdr));
    CMD_LIST->RegisterCmdHandler("pool",            CMD_HANDLER(as_qword));
    CMD_LIST->RegisterCmdHandler("bigpool",         CMD_HANDLER(bigpool));
    CMD_LIST->RegisterCmdHandler("poolrange",       CMD_HANDLER(poolrange));
    CMD_LIST->RegisterCmdHandler("pooltrack",       CMD_HANDLER(pooltrack));
    CMD_LIST->RegisterCmdHandler("poolmetrics",     CMD_HANDLER(poolmetrics));
    CMD_LIST->RegisterCmdHandler("free_pool",       CMD_HANDLER(free_pool));
    CMD_LIST->RegisterCmdHandler("as_mem",          CMD_HANDLER(as_mem));
    CMD_LIST->RegisterCmdHandler("ex_mem",          CMD_HANDLER(ex_mem));
    CMD_LIST->RegisterCmdHandler("page_2_svg",      CMD_HANDLER(page_2_svg));
    CMD_LIST->RegisterCmdHandler("sessions",        CMD_HANDLER(sessions));
    CMD_LIST->RegisterCmdHandler("sessionpool",     CMD_HANDLER(sessionpool));
    CMD_LIST->RegisterCmdHandler("args",            CMD_HANDLER(args));
    CMD_LIST->RegisterCmdHandler("handle_table",    CMD_HANDLER(handle_table));
    CMD_LIST->RegisterCmdHandler("khandles",        CMD_HANDLER(khandles));
    CMD_LIST->RegisterCmdHandler("link",            CMD_HANDLER(link));
    CMD_LIST->RegisterCmdHandler("flink",           CMD_HANDLER(flink));
    CMD_LIST->RegisterCmdHandler("kall",            CMD_HANDLER(kall));
    CMD_LIST->RegisterCmdHandler("pkall",           CMD_HANDLER(pkall));
    CMD_LIST->RegisterCmdHandler("memcpy",          CMD_HANDLER(memcpy));
    CMD_LIST->RegisterCmdHandler("peguid",          CMD_HANDLER(peguid));
    CMD_LIST->RegisterCmdHandler("ls_model",        CMD_HANDLER(ls_model));
    CMD_LIST->RegisterCmdHandler("ls_sessions",     CMD_HANDLER(ls_sessions));
    CMD_LIST->RegisterCmdHandler("ls_processes",    CMD_HANDLER(ls_processes));
    CMD_LIST->RegisterCmdHandler("ls_threads",      CMD_HANDLER(ls_threads));
    CMD_LIST->RegisterCmdHandler("ls_modules",      CMD_HANDLER(ls_modules));
    CMD_LIST->RegisterCmdHandler("ls_handles",      CMD_HANDLER(ls_handles));
    CMD_LIST->RegisterCmdHandler("ps_ttd",          CMD_HANDLER(ps_ttd));
    CMD_LIST->RegisterCmdHandler("session_ttd",     CMD_HANDLER(session_ttd));
    CMD_LIST->RegisterCmdHandler("ttd_calls",       CMD_HANDLER(ttd_calls));
    CMD_LIST->RegisterCmdHandler("ttd_mem_access",  CMD_HANDLER(ttd_mem_access));
    CMD_LIST->RegisterCmdHandler("ttd_mem_use",     CMD_HANDLER(ttd_mem_use));
    CMD_LIST->RegisterCmdHandler("cur_context",     CMD_HANDLER(cur_context));
    CMD_LIST->RegisterCmdHandler("exec",            CMD_HANDLER(exec));
    CMD_LIST->RegisterCmdHandler("mobj",            CMD_HANDLER(mobj));
    CMD_LIST->RegisterCmdHandler("mobj_at",         CMD_HANDLER(mobj_at));
    CMD_LIST->RegisterCmdHandler("call",            CMD_HANDLER(call));
    CMD_LIST->RegisterCmdHandler("ccall",           CMD_HANDLER(ccall));
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

vector<string> CCmdList::GetValidCommands()
{
    vector<string> valid_cmds;

    for (auto cmd : m_cmd_hanlders)
        valid_cmds.push_back(cmd.first);

    return valid_cmds;
}
