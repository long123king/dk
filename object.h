#pragma once

#include "CmdInterface.h"

DECLARE_CMD(obj)
DECLARE_CMD(gobj)
DECLARE_CMD(obj_dir)

void dump_obj(size_t obj_addr, bool b_simple = false);
void dump_obj_dir(size_t obj_hdr_addr, size_t level = 0, bool recurse = false);
uint8_t real_index(size_t type_index, size_t obj_hdr_addr);

wstring dump_obj_name(size_t obj_hdr_addr);
wstring dump_file_name(size_t file_obj_addr);

wstring getTypeName(size_t index);
wstring dump_sym_link(size_t addr);