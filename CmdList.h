#pragma once

#include <string>
#include <vector>
#include <map>
#include <Windows.h>

#include "CmdInterface.h"

using namespace std;

BOOL WINAPI CmdListMain(
    HANDLE Instance, ULONG Reason, PVOID Reserved);

#define CMD_LIST CCmdList::Instance()

class CCmdList
{
public:
    static CCmdList* Instance()
    {
        static CCmdList s_cmd_list;
        return &s_cmd_list;
    }

    void RegisterCmdHandler(string cmd_name, CmdHandler cmd_handler)
    {
        m_cmd_hanlders[cmd_name] = cmd_handler;
    }

    bool IsValidCmd(string cmd_name)
    {
        return m_cmd_hanlders.find(cmd_name) != m_cmd_hanlders.end();
    }

    void HandleCmd(string cmd_name, vector<string>& args)
    {
        m_cmd_hanlders[cmd_name](args);
    }

    vector<string> GetValidCommands();

private:
    map<string, CmdHandler> m_cmd_hanlders;
};
