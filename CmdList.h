#pragma once

#include <string>
#include <vector>
#include <map>
#include <Windows.h>

#include "CmdInterface.h"

BOOL WINAPI CmdListMain(
    HANDLE Instance, ULONG Reason, PVOID Reserved);

#define CMD_LIST CCmdList::Instance()

struct CmdEntry
{
    CmdHandler handler;
    std::string description;
    std::string usage;
};

class CCmdList
{
public:
    static CCmdList* Instance()
    {
        static CCmdList s_cmd_list;
        return &s_cmd_list;
    }

    void RegisterCmdHandler(std::string cmd_name, CmdHandler cmd_handler, std::string description = "", std::string usage = "")
    {
        CmdEntry entry;
        entry.handler = cmd_handler;
        entry.description = description;
        entry.usage = usage;
        m_cmd_handlers[cmd_name] = entry;
    }

    bool IsValidCmd(std::string cmd_name)
    {
        return m_cmd_handlers.find(cmd_name) != m_cmd_handlers.end();
    }

    void HandleCmd(std::string cmd_name, std::vector<std::string>& args)
    {
        m_cmd_handlers[cmd_name].handler(args);
    }

    void PrintUsage(std::string cmd_name);
    void PrintAllHelp();

    std::vector<std::string> GetValidCommands();

private:
    std::map<std::string, CmdEntry> m_cmd_handlers;
};
