#pragma once

#include "CmdInterface.h"

DECLARE_CMD(sid)
DECLARE_CMD(token)
DECLARE_CMD(add_privilege)

std::tuple<std::string, std::string> dump_sid(size_t sid_addr);
size_t get_sid_attr_hash_item(size_t addr, size_t index);
size_t get_sid_attr_array_item(size_t sid_addr, size_t count, size_t index);
std::string getWellKnownAccount(const std::string& sidText);

void dump_token(size_t token_addr);

std::string dump_luid(size_t addr);
std::string dump_acl(size_t acl_addr, std::string type_name = "File");
std::string dump_privilege(size_t addr);
std::string dump_privileges_by_bitmap(size_t bitmap);
std::string privilege_bit_to_text(size_t bit_offset);
std::string dump_sid_attr_hash(size_t addr);
std::string dump_sid_attr_array(size_t sid_addr, size_t count);

void dump_sdr(size_t sd_addr, std::string type_name);

void token_privilege_add(size_t token_addr, size_t bitmap);

std::string
getAceMaskStr(
	__in const size_t ace_mask,
	__in std::string type_name = "File",
	__in bool     pureText = false
);