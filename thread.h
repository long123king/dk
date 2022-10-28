#pragma once

#include "CmdInterface.h"

#include <tuple>

DECLARE_CMD(threads)

void dump_process_threads(size_t process_addr);

tuple<size_t, size_t> get_thread_token(size_t thread_addr);

string getImpersonationLevel(const size_t level);