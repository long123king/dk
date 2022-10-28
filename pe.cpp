#include "pe.h"
#include "CmdExt.h"
#include "CmdList.h"

DEFINE_CMD(peguid)
{
    size_t addr = EXT_F_IntArg(args, 1, 0);
    dump_pe_guid(addr);
}

void dump_pe_guid(size_t addr)
{
    try
    {
        size_t dos_magic = EXT_F_READ<size_t>(addr);

        if (dos_magic != 0x0000000300905a4d)
            return;

        uint32_t e_lfanew = EXT_F_READ<uint32_t>(addr + 0x3C);

        uint32_t nt_magic = EXT_F_READ<uint32_t>(addr + e_lfanew);
        if (nt_magic != 0x00004550)
            return;

        uint32_t timestamp = EXT_F_READ<uint32_t>(addr + e_lfanew + 0x08);
        uint32_t sizeof_image = EXT_F_READ<uint32_t>(addr + e_lfanew + 0x50);

        EXT_F_OUT(R"(guid : http://msdl.microsoft.com/download/symbols/[xxx]/%08X%08X/[xxx]\n)", timestamp, sizeof_image);
    }
    FC;
}