#pragma once

#include "CmdInterface.h"

DECLARE_CMD(link)
DECLARE_CMD(flink)

void dig_link(size_t head, size_t stop_at_entry = 0);