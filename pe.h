#pragma once

#include "CmdInterface.h"

DECLARE_CMD(peguid)
DECLARE_CMD(pe_hdr)

void dump_pe_guid(size_t addr);
void dump_pe_hdr(size_t addr);