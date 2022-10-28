#pragma once

#include <map>
#include <string>
#include <iomanip>
#include <sstream>

using namespace std;

class CBitFieldAnalyzer
{
public:
    CBitFieldAnalyzer()
    {}

    CBitFieldAnalyzer(
        __in const map<uint32_t, const char*>& definitions
    )
    {
        for (auto it = definitions.begin();
            it != definitions.end();
            it++)
        {
            m_definitions[it->first] = it->second;
        }
    }

    CBitFieldAnalyzer(
        __in map<uint32_t, const char*>&& definitions
    )
        :m_definitions(move(definitions))
    {
    }

    string
        GetText(
            __in const uint32_t compound,
            __in bool pureText = false
        )
    {
        stringstream ss;
        if (!pureText)
            ss << "0x" << hex << noshowbase << setw(8) << setfill('0') << compound << "(";

        for (auto it = m_definitions.begin();
            it != m_definitions.end();
            it++)
        {
            if (it->first & compound)
                ss << it->second << ",";
        }

        if (!pureText)
            ss << ")";

        return ss.str();
    }

private:
    map<uint32_t, const char*> m_definitions;
};
