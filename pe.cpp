#include "pe.h"
#include "CmdExt.h"
#include "CmdList.h"

#include <Windows.h>

DEFINE_CMD(peguid)
{
    size_t addr = EXT_F_IntArg(args, 1, 0);
    dump_pe_guid(addr);
}

DEFINE_CMD(pe_hdr)
{
    size_t addr = EXT_F_IntArg(args, 1, 0);
    dump_pe_hdr(addr);
}

std::unique_ptr<IMAGE_DOS_HEADER> addr_to_dos_hdr(size_t addr)
{
    auto dos_hdr = std::make_unique<IMAGE_DOS_HEADER>();
    EXT_D_IDebugDataSpaces->ReadVirtual(addr, dos_hdr.get(), sizeof(*dos_hdr), NULL);
    return dos_hdr;
}

void dump_pe_hdr(size_t addr)
{
    try
    {
        auto dos_hdr = addr_to_dos_hdr(addr);

        if (dos_hdr->e_magic != 0x5A4D)
            return;

        uint32_t e_lfanew = dos_hdr->e_lfanew;

        IMAGE_NT_HEADERS64 pe_hdr = { 0, };

        EXT_D_IDebugDataSpaces->ReadVirtual(addr + e_lfanew, &pe_hdr, sizeof(pe_hdr), NULL);

        if (pe_hdr.Signature != 0x4550)
            return;

        uint32_t timestamp = pe_hdr.FileHeader.TimeDateStamp;
        uint32_t sizeof_image = pe_hdr.OptionalHeader.SizeOfImage;
        uint32_t number_of_sections = pe_hdr.FileHeader.NumberOfSections;

        std::stringstream dml_ss;

        dml_ss << DML_CMD << "!dk pages_2_svg " << std::hex << std::showbase << addr << " " << (sizeof_image + 0xFFF) / 0x1000 << " 1"
            << DML_TEXT << "pages"
            << DML_END
            << std::endl;

        EXT_F_STR_DML(dml_ss);

        if (pe_hdr.FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
        {
            EXT_F_OUT("32-bit PE\n");

            uint32_t sections_offset = e_lfanew + sizeof(IMAGE_NT_HEADERS32);

            EXT_F_OUT("%20s : 0x%016X\n", "PE Header", addr);
            EXT_F_OUT("%20s : 0x%016X\n", "Timestamp", timestamp);
            EXT_F_OUT("%20s : 0x%016X\n", "SizeOfImage", sizeof_image);
            EXT_F_OUT("%20s : %d\n", "NumberOfSections", number_of_sections);

            for (uint32_t i = 0; i < number_of_sections; i++)
            {
                IMAGE_SECTION_HEADER section_hdr = { 0, };
                EXT_D_IDebugDataSpaces->ReadVirtual(addr + sections_offset + i * sizeof(IMAGE_SECTION_HEADER), &section_hdr, sizeof(section_hdr), NULL);

                uint32_t section_va = section_hdr.VirtualAddress;
                uint32_t section_size = section_hdr.Misc.VirtualSize;
                uint32_t section_offset = section_hdr.PointerToRawData;
                uint32_t section_raw_size = section_hdr.SizeOfRawData;

                EXT_F_OUT("\t[%d] %10s : VA=0x%016X, Size=0x%016X, Offset=0x%016X, RawSize=0x%016X\n", i, section_hdr.Name, section_va, section_size, section_offset, section_raw_size);
            }
        }
        else if (pe_hdr.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
        {
            EXT_F_OUT("64-bit PE\n");

            uint32_t sections_offset = e_lfanew + sizeof(IMAGE_NT_HEADERS64);

            EXT_F_OUT("%20s : 0x%016X\n", "PE Header", addr);
            EXT_F_OUT("%20s : 0x%016X\n", "Timestamp", timestamp);
            EXT_F_OUT("%20s : 0x%016X\n", "SizeOfImage", sizeof_image);
            EXT_F_OUT("%20s : %d\n", "NumberOfSections", number_of_sections);

            for (uint32_t i = 0; i < number_of_sections; i++)
            {
                IMAGE_SECTION_HEADER section_hdr = { 0, };
                EXT_D_IDebugDataSpaces->ReadVirtual(addr + sections_offset + i * sizeof(IMAGE_SECTION_HEADER), &section_hdr, sizeof(section_hdr), NULL);

                uint32_t section_va = section_hdr.VirtualAddress;
                uint32_t section_size = section_hdr.Misc.VirtualSize;
                uint32_t section_offset = section_hdr.PointerToRawData;
                uint32_t section_raw_size = section_hdr.SizeOfRawData;

                EXT_F_OUT("\t[%d] %10s : VA=0x%016X, Size=0x%016X, Offset=0x%016X, RawSize=0x%016X\n", i, section_hdr.Name, section_va, section_size, section_offset, section_raw_size);
            }

            
        }
        else
        {
            EXT_F_OUT("Unknown PE\n");
        }




    }
    FC;
}

void dump_pe_guid(size_t addr)
{
    try
    {
        auto dos_hdr = addr_to_dos_hdr(addr);

        if (dos_hdr->e_magic != 0x5A4D)
            return;

        uint32_t e_lfanew = dos_hdr->e_lfanew;

        IMAGE_NT_HEADERS64 pe_hdr = { 0, };

        EXT_D_IDebugDataSpaces->ReadVirtual(addr + e_lfanew, &pe_hdr, sizeof(pe_hdr), NULL);

        if (pe_hdr.Signature != 0x4550)
            return;

        uint32_t timestamp = pe_hdr.FileHeader.TimeDateStamp;
        uint32_t sizeof_image = pe_hdr.OptionalHeader.SizeOfImage;

        //size_t dos_magic = EXT_F_READ<size_t>(addr);

        //if (dos_magic != 0x0000000300905a4d)
        //    return;

        //uint32_t e_lfanew = EXT_F_READ<uint32_t>(addr + 0x3C);

        //uint32_t nt_magic = EXT_F_READ<uint32_t>(addr + e_lfanew);
        //if (nt_magic != 0x00004550)
        //    return;

        //uint32_t timestamp = EXT_F_READ<uint32_t>(addr + e_lfanew + 0x08);
        //uint32_t sizeof_image = EXT_F_READ<uint32_t>(addr + e_lfanew + 0x50);

        EXT_F_OUT(R"(guid : http://msdl.microsoft.com/download/symbols/[xxx]/%08X%08X/[xxx]\n)", timestamp, sizeof_image);
    }
    FC;
}