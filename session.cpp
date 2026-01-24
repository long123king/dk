#include "session.h"
#include "CmdExt.h"
#include "CmdList.h"
#include "object.h"
#include "token.h"
#include "process.h"
#include <iomanip>

DEFINE_CMD(sessions)
{
    dump_logon_sessions();
}

DEFINE_CMD(sessionpool)
{
    dump_session_pool();
}

void dump_logon_sessions()
{
    try
    {
        size_t logon_sessions_addr = EXT_F_Sym2Addr("nt!SepLogonSessions");
        if (logon_sessions_addr == 0)
            return;

        size_t logon_sessions_table_addr = EXT_F_READ<size_t>(logon_sessions_addr);
        if (logon_sessions_table_addr == 0)
            return;

        for (size_t i = 0; i < 0x10; i++)
        {
            size_t logon_session_addr = EXT_F_READ<size_t>(logon_sessions_table_addr + i * 8);
            if (logon_session_addr == 0)
                continue;

            EXT_F_OUT("#%x:\n", i);
            dump_session(logon_session_addr);
        }
    }
    FC;
}

void dump_session(size_t session_addr)
{
    if (session_addr == 0)
        return;

    try
    {
        ExtRemoteTyped session("(nt!_SEP_LOGON_SESSION_REFERENCES*)@$extin", session_addr);
        EXT_F_OUT("Session: 0x%I64x\n", session_addr);
        EXT_F_OUT("%16s: %s\n", "LogonId", dump_luid(session_addr + session.GetFieldOffset("LogonId")).c_str());
        EXT_F_OUT("%16s: %s\n", "BuddyLogonId", dump_luid(session_addr + session.GetFieldOffset("BuddyLogonId")).c_str());
        EXT_F_OUT("%16s: 0x%I64x\n", "ReferenceCount", session.Field("ReferenceCount").GetUlong64());
        EXT_F_OUT(L"%16s: %s\n", L"AccountName", EXT_F_READ_USTR(session_addr + session.GetFieldOffset("AccountName")).c_str());
        EXT_F_OUT(L"%16s: %s\n", L"AuthorityName", EXT_F_READ_USTR(session_addr + session.GetFieldOffset("AuthorityName")).c_str());

        EXT_F_OUT("%16s: 0x%I64x\n", "Token", session.Field("Token").GetUlongPtr());
    }
    FC
}

void dump_session_pool()
{
    try
    {
        size_t proc_addr = curr_proc();
        size_t session_addr = EXT_F_READ<size_t>(proc_addr + 0x400);

        size_t this_session_addr = session_addr;
        dump_session_space(this_session_addr);
        session_addr = EXT_F_READ<size_t>(session_addr + 0x98) - 0x90;
        while (session_addr != this_session_addr)
        {
            if ((session_addr & 0x00000FFF) == 0x000)
                dump_session_space(session_addr);

            session_addr = EXT_F_READ<size_t>(session_addr + 0x98) - 0x90;
        }
    }
    FC;
}


void dump_session_space(size_t addr)
{
    uint32_t session_id = EXT_F_READ<uint32_t>(addr + 0x08);

    EXT_F_OUT("Session : %d\n", session_id);

    std::stringstream ss;
    ss.str("");
    ss << "dt nt!_POOL_DESCRIPTOR "
        << std::hex << std::showbase << addr + 0xcc0
        << "";

    EXT_F_EXEC(ss.str());
}