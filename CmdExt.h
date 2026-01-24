#pragma once

#include <map>
#include <string>
#include <sstream>
#include <vector>

#include <Windows.h>

#define INITGUID
#define EXT_CLASS CCmdExt 
#include <engextcpp.hpp> 

#define s_CmdExt                      CCmdExt::Instance()

#define EXT_F_READ                    s_CmdExt->read
#define EXT_F_WRITE                   s_CmdExt->write
#define EXT_F_READX                   s_CmdExt->readX
#define EXT_F_EXEC                    s_CmdExt->x
#define EXT_F_IntArg                  s_CmdExt->getIntArg
#define EXT_F_ValidAddr               s_CmdExt->valid_addr
#define EXT_F_Sym2Addr                s_CmdExt->getSymbolAddr
#define EXT_F_Addr2Sym                s_CmdExt->getAddrSymbol
#define EXT_F_REG                     s_CmdExt->reg
#define EXT_F_REG_OF                  s_CmdExt->reg_of
#define EXT_F_IS_IN_RANGE             s_CmdExt->is_in_range
#define EXT_F_Size2Str                s_CmdExt->size2str
#define EXT_F_READ_USTR               s_CmdExt->readUnicodeString
#define EXT_F_THROW                   s_CmdExt->ThrowRemote

#define EXT_F_OUT                     s_CmdExt->Out
#define EXT_F_ERR                     s_CmdExt->Err
#define EXT_F_DML                     s_CmdExt->Dml
#define EXT_F_STR_OUT                 s_CmdExt->out_str
#define EXT_F_STR_ERR                 s_CmdExt->err_str
#define EXT_F_STR_DML                 s_CmdExt->dml_str
#define EXT_F_DML_LINK                s_CmdExt->DmlCmdLink
#define EXT_F_DML_EXEC                s_CmdExt->DmlCmdExec

#define EXT_D_IDebugAdvanced          s_CmdExt->m_Advanced
#define EXT_D_IDebugClient            s_CmdExt->m_Client
#define EXT_D_IDebugControl           s_CmdExt->m_Control
#define EXT_D_IDebugDataSpaces        s_CmdExt->m_Data
#define EXT_D_IDebugRegisters         s_CmdExt->m_Registers
#define EXT_D_IDebugSymbols           s_CmdExt->m_Symbols
#define EXT_D_IDebugSystemObjects     s_CmdExt->m_System
#define EXT_D_IDebugAdvanced2         s_CmdExt->m_Advanced2
#define EXT_D_IDebugAdvanced3         s_CmdExt->m_Advanced3
#define EXT_D_IDebugClient2           s_CmdExt->m_Client2
#define EXT_D_IDebugClient3           s_CmdExt->m_Client3
#define EXT_D_IDebugClient4           s_CmdExt->m_Client4
#define EXT_D_IDebugClient5           s_CmdExt->m_Client5
#define EXT_D_IDebugControl2          s_CmdExt->m_Control2
#define EXT_D_IDebugControl3          s_CmdExt->m_Control3
#define EXT_D_IDebugControl4          s_CmdExt->m_Control4
#define EXT_D_IDebugControl5          s_CmdExt->m_Control5
#define EXT_D_IDebugControl6          s_CmdExt->m_Control6
#define EXT_D_IDebugDataSpaces2       s_CmdExt->m_Data2
#define EXT_D_IDebugDataSpaces3       s_CmdExt->m_Data3
#define EXT_D_IDebugDataSpaces4       s_CmdExt->m_Data4
#define EXT_D_IDebugRegisters2        s_CmdExt->m_Registers2
#define EXT_D_IDebugSymbols2          s_CmdExt->m_Symbols2
#define EXT_D_IDebugSymbols3          s_CmdExt->m_Symbols3
#define EXT_D_IDebugSystemObjects2    s_CmdExt->m_System2
#define EXT_D_IDebugSystemObjects3    s_CmdExt->m_System3
#define EXT_D_IDebugSystemObjects4    s_CmdExt->m_System4

#define DML_CMD     "<link cmd=\""
#define DML_TEXT    "\">"
#define DML_END     "</link>"

#define WDML_CMD     "<link cmd=\""
#define WDML_TEXT    "\">"
#define WDML_END     "</link>"

#define HEX_ADDR(addr) hex << showbase << setfill('0') << setw(18) << addr << setfill(' ')
#define WHEX_ADDR(addr) hex << showbase << setfill(L'0') << setw(18) << addr << setfill(L' ')
#define HEX_DATA hex << showbase << setfill(' ') << setw(18)
#define WHEX_DATA hex << showbase << setfill(L' ') << setw(18)

#define FC \
    catch (ExtException& e)     \
    {                            \
        EXT_F_ERR("[ERROR] %s %s %d %s\n", __FILE__, __FUNCTION__, __LINE__, e.GetMessage());       \
    }                          \
    catch (std::exception& e)        \
    {           \
        EXT_F_ERR("[ERROR] %s %s %d %s\n", __FILE__, __FUNCTION__, __LINE__, e.what());       \
    }

class CCmdExt 
    : public ExtExtension
{
public:
    static CCmdExt* Instance()
    {
        static CCmdExt s_cmd_ext;
        return &s_cmd_ext;
    }

    CCmdExt();
    ~CCmdExt();        

    void dk(void);       

    size_t reg(size_t reg);

    size_t reg_of(const char* reg);

    bool is_reg(std::string& str);

    size_t getSymbolAddr(const char* name);

    std::tuple<std::string, uint64_t> getAddrSymbol(size_t addr);

    bool valid_addr(size_t addr);

    size_t getIntArg(std::vector<std::string>& args, size_t idx, size_t default_val);

    template<class T>
    inline bool is_in_range(T value, T min, T max)
    {
        return value >= min && value < max;
    }
    
    template<typename T>
    T read(size_t addr);

    template<typename T>
    T readX(size_t addr);

    template<typename T>
    void write(size_t addr, T data);         

    std::string size2str(size_t value);      

    std::wstring readUnicodeString(size_t addr);

    HRESULT x(std::string cmd);

    void out_str(std::stringstream& ss)
    {
        Out(ss.str().c_str());
    }

    void dml_str(std::stringstream& ss)
    {
        Dml(ss.str().c_str());
    }

    void err_str(std::stringstream& ss)
    {
        Err(ss.str().c_str());
    }
};

template<typename T>
inline T CCmdExt::read(size_t addr)
{
    T ret = 0;
    if (S_OK != m_Data->ReadVirtual(addr, &ret, sizeof(T), NULL))
        ThrowRemote(E_ACCESSDENIED, "Fail to read memory");

    return ret;
}

template<typename T>
inline T CCmdExt::readX(size_t addr)
{
    T ret = 0;
    if (S_OK != m_Data->ReadPhysical(addr, &ret, sizeof(T), NULL))
        ThrowRemote(E_ACCESSDENIED, "Fail to read memory");

    return ret;
}

template<typename T>
inline void CCmdExt::write(size_t addr, T data)
{
    if (S_OK != m_Data->WriteVirtual(addr, &data, sizeof(T), NULL))
        ThrowRemote(E_ACCESSDENIED, "Fail to write memory");
}

