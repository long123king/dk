#include "type.h"
#include "CmdList.h"
#include "CmdExt.h"
#include "object.h"
#include "token.h"

#pragma warning( disable : 4244)

DEFINE_CMD(types)
{
    dump_types();
}

void dump_types()
{
    try
    {
        size_t types_table = EXT_F_Sym2Addr("nt!ObpObjectTypes");

        for (size_t i = 0; i < 0xFF; i++)
        {
            size_t type_addr = EXT_F_READ<size_t>(types_table + i * sizeof(size_t));

            if (type_addr == 0)
                break;

            ExtRemoteTyped type("(nt!_OBJECT_TYPE*)@$extin", type_addr);
            std::wstring name = EXT_F_READ_USTR(type_addr + type.GetFieldOffset("Name"));
            //wstring name = readUnicodeString(type.Field("Name").GetPtr());
            uint32_t num_objects = type.Field("TotalNumberOfObjects").GetUlong();
            uint32_t num_handles = type.Field("TotalNumberOfHandles").GetUlong();
            uint32_t valid_access_mask = type.Field("TypeInfo.ValidAccessMask").GetUlong();
            static const char* routines[]{
                    "DumpProcedure",
                    "OpenProcedure",
                    "CloseProcedure",
                    "DeleteProcedure",
                    "ParseProcedure",
                    "SecurityProcedure",
                    "QueryNameProcedure",
                    "OkayToCloseProcedure"
            };

            EXT_F_OUT("---------------------------------------------------------\n");
            EXT_F_OUT(L"%02X objs: 0x%08x, handles: 0x%08x  0x%08x %s\n", i, num_objects, num_handles, valid_access_mask, name.c_str());
            EXT_F_OUT("[ Routines: ]\n");
            for (auto& routine : routines)
            {
                std::string field = "TypeInfo.";
                field += routine;
                size_t routine_addr = type.Field(field.c_str()).GetUlongPtr();
                std::string routine_name = get<0>(EXT_F_Addr2Sym(routine_addr));
                EXT_F_OUT("\t%20s : 0x%I64x %s\n", routine, routine_addr, routine_name.c_str());
            }

            std::string strname(name.begin(), name.end());

            uint32_t gR_access = type.Field("TypeInfo.GenericMapping.GenericRead").GetUlong();
            uint32_t gW_access = type.Field("TypeInfo.GenericMapping.GenericWrite").GetUlong();
            uint32_t gE_access = type.Field("TypeInfo.GenericMapping.GenericExecute").GetUlong();
            uint32_t gA_access = type.Field("TypeInfo.GenericMapping.GenericAll").GetUlong();

            EXT_F_OUT("[ Generic Mapping: ]\n\t%20s : 0x%08x %s\n\t%20s : 0x%08x %s\n\t%20s : 0x%08x %s\n\t%20s : 0x%08x %s\n",
                "GenericRead", gR_access, getAceMaskStr(gR_access, strname).c_str(),
                "GenericWrite", gW_access, getAceMaskStr(gW_access, strname).c_str(),
                "GenericExecute", gE_access, getAceMaskStr(gE_access, strname).c_str(),
                "GenericAll", gA_access, getAceMaskStr(gA_access, strname).c_str());
        }
    }
    FC;
}