#pragma once

#include "CmdInterface.h"

DECLARE_CMD(lmu)
DECLARE_CMD(lmk)
DECLARE_CMD(lm)

void dump_user_modules();
void dump_kernel_modules();
void dump_modules();