#pragma once

#include "CmdInterface.h"

DECLARE_CMD(link)
DECLARE_CMD(flink)


void dig_link(size_t addr);

void dig_link(size_t addr, size_t head);

size_t dig_link(size_t addr, size_t head, size_t entry, size_t index, size_t count);