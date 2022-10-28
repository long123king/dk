#pragma once

#include "CmdInterface.h"

DECLARE_CMD(sessions)
DECLARE_CMD(sessionpool)

void dump_logon_sessions();
void dump_session(size_t session_addr);
void dump_session_pool();
void dump_session_space(size_t addr);