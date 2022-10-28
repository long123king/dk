#pragma once

#include "CmdInterface.h"

DECLARE_CMD(kall)
DECLARE_CMD(pkall)

void dump_threads_stack(size_t process_addr);
void dump_all_threads_stack();