#pragma once

#include "CmdInterface.h"

DECLARE_CMD(pses)
DECLARE_CMD(ps_flags);
DECLARE_CMD(kill);
DECLARE_CMD(process);

void dump_process(size_t process_addr);
void dump_ps_flags(size_t addr);
size_t get_process_silo(size_t process_addr);
size_t get_job_silo(size_t job_addr);
std::string getProtectionText(uint8_t protection);
std::string getTokenIL(size_t token_addr);

size_t get_cr3();

size_t curr_proc();
size_t curr_thread();
size_t curr_tid();
size_t curr_token();

void kill_process(size_t proc_addr);