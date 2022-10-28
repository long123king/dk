#include "module.h"
#include "CmdList.h"
#include "CmdExt.h"
#include "object.h"
#include "dbgdata.h"

DEFINE_CMD(lmu)
{
    dump_user_modules();
}

DEFINE_CMD(lmk)
{
    dump_kernel_modules();
}

DEFINE_CMD(lm)
{
    dump_modules();
}

void dump_user_modules()
{
    ExtRemoteTypedList lm_list = ExtNtOsInformation::GetUserLoadedModuleList();

    for (lm_list.StartHead(); lm_list.HasNode(); lm_list.Next())
    {
        ExtRemoteTyped lm = lm_list.GetTypedNode();
        size_t name_addr = lm_list.GetNodeOffset() + lm.GetFieldOffset("FullDllName");
        size_t dll_base = lm.Field("DllBase").GetUlong64();
        EXT_F_OUT(L"0x%0I64x %s\n", dll_base, EXT_F_READ_USTR(name_addr).c_str());
    }
}

void dump_kernel_modules()
{

    ExtRemoteTypedList klm_list = ExtNtOsInformation::GetKernelLoadedModuleList();

    for (klm_list.StartHead(); klm_list.HasNode(); klm_list.Next())
    {
        ExtRemoteTyped lm = klm_list.GetTypedNode();
        size_t name_addr = klm_list.GetNodeOffset() + lm.GetFieldOffset("FullDllName");
        size_t dll_base = lm.Field("DllBase").GetUlong64();
        EXT_F_OUT(L"0x%0I64x %s\n", dll_base, EXT_F_READ_USTR(name_addr).c_str());
    }
}

void dump_modules()
{
    try
    {
        size_t lm_head_addr = readDbgDataAddr(DEBUG_DATA_PsLoadedModuleListAddr);

        ExtRemoteTypedList modules_list(lm_head_addr, "nt!_LDR_DATA_TABLE_ENTRY", "InLoadOrderLinks");
        for (modules_list.StartHead(); modules_list.HasNode(); modules_list.Next())
        {
            auto module = modules_list.GetTypedNode();
            size_t module_addr = modules_list.GetNodeOffset();

            size_t base_addr = module.Field("DllBase").GetUlongPtr();
            wstring full_name = EXT_F_READ_USTR(module_addr + module.GetFieldOffset("FullDllName"));
            wstring base_name = EXT_F_READ_USTR(module_addr + module.GetFieldOffset("BaseDllName"));
            size_t entry = module.Field("EntryPoint").GetUlongPtr();
            uint32_t size = module.Field("SizeOfImage").GetUlong();

            EXT_F_OUT(L"0x%I64x [0x%016x] 0x%I64x %20s %s\n", base_addr, size, entry, base_name.c_str(), full_name.c_str());
        }
    }
    FC;
}