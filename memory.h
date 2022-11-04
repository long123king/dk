#pragma once

#include "CmdInterface.h"

#include <tuple>

DECLARE_CMD(size)
DECLARE_CMD(va_regions)
DECLARE_CMD(regs)
DECLARE_CMD(as_qword)
DECLARE_CMD(as_mem)
DECLARE_CMD(ex_mem)
DECLARE_CMD(args)
DECLARE_CMD(vad)
DECLARE_CMD(memcpy)
DECLARE_CMD(page_2_svg)

void dump_size(size_t value);
void dump_args();
void dump_regs();
void analyze_qword(size_t value);
void analyze_mem(size_t start, size_t len, size_t offset = 0);
void extract_mem(size_t start, size_t len, size_t offset = 0);
bool like_kaddr(size_t addr);

bool in_curr_stack(size_t addr);
string va_region_name(size_t addr);
bool in_paged_pool(size_t addr);
bool in_non_paged_pool(size_t addr);
bool in_small_pool_page(size_t addr);
tuple<bool, size_t, string, string> as_kcode(size_t addr);
tuple<bool, size_t, string, string> as_ucode(size_t addr);
tuple<bool, bool, bool, size_t, size_t, string> as_small_pool(size_t addr);
tuple<bool, bool, bool, size_t, size_t, string> as_large_pool(size_t addr);
string dump_plain_qword(size_t curr_qword);

void init_va_regions();
void dump_va_regions();

void dump_vad(size_t root_addr);
void visit_vad(size_t vad_node_addr);

void do_memcpy(size_t src_addr, size_t dst_addr, size_t count);

void page_to_svg(size_t addr, string svg_filename);

void mem_access_to_svg(size_t start_addr, size_t end_addr, string mode, string svg_filename);

