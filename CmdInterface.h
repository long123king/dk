#pragma once

#include <string>
#include <vector>

using namespace std;

using CmdHandler = void (*)(vector<string>& args);

#define DECLARE_CMD(x) void cmd_##x(vector<string>& args);
#define DEFINE_CMD(x) void cmd_##x(vector<string>& args)
#define CMD_HANDLER(x) cmd_##x