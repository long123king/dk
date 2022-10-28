#include "CmdExt.h"   
#include "CmdList.h"

#include <iomanip>

ExtExtension* g_ExtInstancePtr = CCmdExt::Instance();

CCmdExt::CCmdExt()
{ 
    ExtSetDllMain::ExtSetDllMain(CmdListMain);
} 

bool CCmdExt::is_reg(string & str)
{
    static vector<string> regs{ {
            "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rsp", "rbp", "rip",
            "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16"
        } };

    return find(regs.begin(), regs.end(), str.c_str()) != regs.end();
}

void CCmdExt::dk(void)
{
    try
    {
        string raw_args = GetRawArgStr();
        vector<string> args;
        for (size_t i = 0, j = 0; i <= raw_args.size(); i++)
        {
            if (raw_args[i] == ' ' || i == raw_args.size())
            {
                if (i > j)
                    args.push_back(string(raw_args, j, i - j));

                j = i+1;
            }                
        }

        if (!args.empty())
        {
            string cmd = args[0];

            if (CMD_LIST->IsValidCmd(cmd))
                CMD_LIST->HandleCmd(cmd, args);
            else
            {
                Out("Not valid command: !dk %s! Try following commands:\n", cmd.c_str());

                for (auto cmd : CMD_LIST->GetValidCommands())
                {
                    Out("\t!dk %s\n", cmd.c_str());
                }
            }
        }
        else
        {
            Err("Invalid parameters\n");
        }
    }
    FC;
}

CCmdExt::~CCmdExt()
{
} 


size_t CCmdExt::reg(size_t reg)
{
    try
    {
        DEBUG_VALUE reg_val = { 0, };
        m_Registers->GetValue((ULONG)reg, &reg_val);

        return reg_val.I64;
    }
    FC;

    return 0;
}

size_t CCmdExt::getSymbolAddr(const char* name)
{
    size_t addr = 0;
    try
    {
        m_Symbols->GetOffsetByName(name, &addr);
    }
    FC;
    return addr;
}

tuple<string, uint64_t> CCmdExt::getAddrSymbol(size_t addr)
{
    string name;
    ULONG64 disp = 0;
    try
    {
        char* symbol = nullptr;
        ULONG len = 0;
        if (S_OK == m_Symbols->GetNameByOffset(addr, symbol, 0, &len, 0))
        {
            symbol = new (nothrow) char[len];

            if (symbol != nullptr &&
                S_OK == m_Symbols->GetNameByOffset(addr, symbol, len, &len, &disp))
                name = symbol;

            if (symbol != nullptr)
                delete[] symbol;
        }
    }
    FC;
    return make_tuple(name, disp);
}

size_t CCmdExt::reg_of(const char* reg_v)
{
#define REG_RAX     0
#define REG_RCX     1
#define REG_RDX     2
#define REG_RBX     3
#define REG_RSP     4
#define REG_RBP     5
#define REG_RSI     6
#define REG_RDI     7
#define REG_R8      8
#define REG_R9      9
#define REG_R10     10
#define REG_R11     11
#define REG_R12     12
#define REG_R13     13
#define REG_R14     14
#define REG_R15     15
#define REG_RIP     16

    static map<const char*, size_t> regs_map{
        {"rip", REG_RIP}, {"rbp", REG_RBP}, {"rsp", REG_RSP},
        {"rax", REG_RAX}, {"rbx", REG_RBX}, {"rcx", REG_RCX}, {"rdx", REG_RDX},
        {"rsi", REG_RSI}, {"rdi", REG_RDI},
        {"r8", REG_R8},{ "r9", REG_R9 },{ "r10", REG_R10 },{ "r11", REG_R11 },
        { "r12", REG_R12 },{ "r13", REG_R13 },{ "r14", REG_R14 },{ "r15", REG_R15 }
    };

    uint32_t index = 0;
    if (m_Registers->GetIndexByName(reg_v, (PULONG)&index) == S_OK)
    {
        return reg(index);
    }

    if (regs_map.find(reg_v) == regs_map.end())
        return 0;

    return reg(regs_map[reg_v]);

#undef REG_RAX 
#undef REG_RCX 
#undef REG_RDX 
#undef REG_RBX 
#undef REG_RSP 
#undef REG_RBP 
#undef REG_RSI 
#undef REG_RDI 
#undef REG_R8  
#undef REG_R9  
#undef REG_R10 
#undef REG_R11 
#undef REG_R12 
#undef REG_R13 
#undef REG_R14 
#undef REG_R15 
#undef REG_RIP 
}

bool CCmdExt::valid_addr(size_t addr)
{
    bool valid = true;
    try
    {
        read<uint8_t>(addr);
    }
    catch (ExtException)
    {
        valid = false;
    }

    return valid;
}

wstring CCmdExt::readUnicodeString(size_t addr)
{
    wstring name;
    try
    {
        ExtRemoteTyped us("(nt!_UNICODE_STRING*)@$extin", addr);
        size_t name_addr = us.Field("Buffer").GetLongPtr();
        size_t name_len = us.Field("Length").GetUshort();
        wchar_t* obj_name_buf = new wchar_t[name_len / 2 + 1];
        memset(obj_name_buf, 0, (name_len / 2 + 1) * sizeof(wchar_t));
        if (S_OK == EXT_D_IDebugDataSpaces->ReadVirtual(name_addr, obj_name_buf, (ULONG)name_len, NULL))
            name = obj_name_buf;

        delete[] obj_name_buf;
    }
    FC;

    return name;
}

string CCmdExt::size2str(size_t value)
{
    string str("");
    try
    {
        size_t k_size = value / 1024;
        size_t m_size = k_size / 1024;
        size_t g_size = m_size / 1024;
        size_t t_size = g_size / 1024;

        stringstream ss;

        /*ss << showbase << hex;

        if (t_size != 0)
            ss << setw(6) << t_size << " T ";

        if (g_size != 0)
            ss << setw(6) << g_size - t_size * 1024 << " G ";

        if (m_size != 0)
            ss << setw(6) << m_size - g_size * 1024 << " M ";

        if (k_size != 0)
            ss << setw(6) << k_size - m_size * 1024 << " K ";

        ss << value - k_size * 1024;

        ss << endl;*/
        ss << noshowbase << dec;

        if (t_size != 0)
            ss << setw(6) << t_size << " T ";

        if (g_size != 0)
            ss << setw(6) << g_size - t_size * 1024 << " G ";

        if (m_size != 0)
            ss << setw(6) << m_size - g_size * 1024 << " M ";

        if (k_size != 0)
            ss << setw(6) << k_size - m_size * 1024 << " K ";

        ss << value - k_size * 1024;

        //ss << endl;

        str = ss.str();
    }
    FC;

    return str;
}

HRESULT CCmdExt::x(string cmd)
{
    try
    {
        return m_Control->Execute(DEBUG_OUTCTL_ALL_CLIENTS, cmd.c_str(), DEBUG_EXECUTE_ECHO);
    }
    FC;

    return S_FALSE;
}

size_t CCmdExt::getIntArg(vector<string>& args, size_t idx, size_t default_val)
{
    if (idx < 0 || idx >= args.size())
        return default_val;

    stringstream ss;

    ss << nouppercase << args[idx];

    string arg = ss.str();

    if (is_reg(arg))
    {
        return reg_of(arg.c_str());
    }

    if (arg.size() == 17 && arg[8] == '`')
    {
        for (size_t i = 8; i < 17; i++)
            arg[i] = arg[i + 1];
        arg.resize(16);
    }
    else if (arg.size() == 19 && arg[10] == '`' && arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X'))
    {
        for (size_t i = 10; i < 19; i++)
            arg[i] = arg[i + 1];
        arg.resize(18);
    }

    bool b_hex = true;
    if (arg[0] == '0')
    {
        if (arg[1] == 'n' || arg[1] == 'N')
        {
            arg = arg.substr(2);  
            b_hex = false;
        }
        else if (arg[1] == 'x' || arg[1] == 'X')
        {
            arg = arg.substr(2);
            b_hex = true;
        }
    }

    try
    {
        size_t val = strtoull(arg.c_str(), nullptr, b_hex ? 16 : 10);
        if (val == 0)
            return default_val;

        return val;
    }
    catch (exception)
    {
        Err("Invalid conversion!\n");
    }

    return default_val;
}

ExtCommandDesc g_dk_desc(
    "dk",
    (ExtCommandMethod)&CCmdExt::dk,
    "Customized command line parse entry",
    "{{custom}}");

extern "C"
HRESULT
CALLBACK
dk(
    __in        PDEBUG_CLIENT   client,
    __in_opt    PCSTR           args)
{
    if (!g_Ext.IsSet())
    {
        return E_UNEXPECTED;
    }
    return g_Ext->CallCommand(&g_dk_desc, client, args);
}
