#pragma once

#include "CmdInterface.h"
#include <Windows.h>

DECLARE_CMD(dbgdata)

size_t readDbgDataAddr(ULONG index);
