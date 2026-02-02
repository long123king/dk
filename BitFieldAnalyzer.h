#pragma once

#include <map>
#include <string>
#include <iomanip>
#include <sstream>



class CBitFieldAnalyzer
{
public:
    CBitFieldAnalyzer()
    {}

    CBitFieldAnalyzer(
        __in const std::map<uint32_t, const char*>& definitions
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
        __in std::map<uint32_t, const char*>&& definitions
    )
        :m_definitions(std::move(definitions))
    {
    }

    std::string
        GetText(
            __in const uint32_t compound,
            __in bool pureText = false
        )
    {
        std::stringstream ss;
        if (!pureText)
            ss << "0x" << std::hex << std::noshowbase << std::setw(8) << std::setfill('0') << compound << "(";

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
    std::map<uint32_t, const char*> m_definitions;
};
