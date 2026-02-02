#pragma once

#include <string>
#include <vector>

using CmdHandler = void (*)(std::vector<std::string>& args);

#define DECLARE_CMD(x) void cmd_##x(std::vector<std::string>& args);
#define DEFINE_CMD(x) void cmd_##x(std::vector<std::string>& args)
#define CMD_HANDLER(x) cmd_##x