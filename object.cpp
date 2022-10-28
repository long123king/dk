#include "object.h"
#include "CmdList.h"
#include "CmdExt.h"
#include "dbgdata.h"
#include "token.h"

#include <string>
#include <iomanip>
#include <cstdint>

#pragma warning( disable : 4244)

using namespace std;

size_t g_header_cookie_addr = 0;
size_t g_type_index_table_addr = 0;
size_t g_ob_header_cookie = 0;

map<uint8_t, wstring> g_type_name_map;

string
getObjectStructByName(
	string type_name
)
{
	string struct_name = "";
	if (type_name == "Token")
		return "nt!_TOKEN";
	else if (type_name == "Process")
		return "nt!_EPROCESS";
	else if (type_name == "Thread")
		return "nt!_ETHREAD";
	else if (type_name == "Directory")
		return "nt!_OBJECT_DIRECTORY";
	else if (type_name == "Section")
		return "nt!_SECTION";
	else if (type_name == "Mutant")
		return "nt!_KMUTANT";
	else if (type_name == "Semaphore")
		return "nt!_KSEMAPHORE";
	else if (type_name == "Event")
		return "nt!_KEVENT";
	else if (type_name == "TmTx")
		return "nt!_KTRANSACTION";
	else if (type_name == "TmTm")
		return "nt!_KTM";
	else if (type_name == "TmRm")
		return "nt!_KRESOURCEMANAGER";
	else if (type_name == "Timer")
		return "nt!_KTIMER";
	else if (type_name == "Job")
		return "nt!_EJOB";
	//else if (type_name == "Key")
	//	generic_mask_str += getKeySpecificAccess(specific_mask, pureText);
	//else if (type_name == "TmEn")
	//	generic_mask_str += getEnlistSpecificAccess(specific_mask, pureText);
	//else if (type_name == "IoCompletion")
	//	generic_mask_str += getIoCSpecificAccess(specific_mask, pureText);
	else if (type_name == "File")
		return "nt!_FILE_OBJECT";

	return "nt!_FILE_OBJECT";
}

DEFINE_CMD(obj)
{
	size_t obj_addr = EXT_F_IntArg(args, 1, 0);

    dump_obj(obj_addr);
}

DEFINE_CMD(gobj)
{
    size_t root_obj_dir_addr = readDbgDataAddr(DEBUG_DATA_ObpRootDirectoryObjectAddr);
    if (root_obj_dir_addr == 0)
        return;

    try
    {
        dump_obj_dir(EXT_F_READ<size_t>(root_obj_dir_addr), 0, true);
    }
    FC;
}

DEFINE_CMD(obj_dir)
{
	size_t addr = EXT_F_IntArg(args, 1, 0);
	dump_obj_dir(addr, 0, true);
}

uint8_t real_index(size_t type_index, size_t obj_hdr_addr)
{	
	if (g_header_cookie_addr == 0 && g_ob_header_cookie == 0 && g_type_index_table_addr == 0)
	{
		if (S_OK != EXT_D_IDebugSymbols->GetOffsetByName("nt!ObHeaderCookie", &g_header_cookie_addr) ||
			S_OK != EXT_D_IDebugDataSpaces->ReadVirtual(g_header_cookie_addr, &g_ob_header_cookie, sizeof(uint8_t), NULL))
		{
			EXT_F_ERR("Fail to get the offset of nt!ObHeaderCookie\n");
			return 0;
		}

		if (S_OK != EXT_D_IDebugSymbols->GetOffsetByName("nt!ObTypeIndexTable", &g_type_index_table_addr))
		{
			EXT_F_ERR("Fail to get the offset of nt!ObTypeIndexTable\n");
			return 0;
		}
	}

	uint8_t byte_2nd_addr = uint8_t(obj_hdr_addr >> 8);
	return (uint8_t)(type_index ^ g_ob_header_cookie ^ byte_2nd_addr);
}

void dump_obj(size_t obj_addr, bool b_simple)
{
	try
	{
		ExtRemoteTyped obj_hdr("(nt!_OBJECT_HEADER*)@$extin", obj_addr);
		//obj_hdr.OutFullValue();

		uint8_t r_index = real_index(obj_hdr.Field("TypeIndex").GetUchar(), obj_addr);
		size_t sdr_addr = obj_hdr.Field("SecurityDescriptor").GetLongPtr() & 0xFFFFFFFFFFFFFFF0;

		wstring obj_name = dump_obj_name(obj_addr);
		wstring type_name = getTypeName(r_index);

		string str_type_name(type_name.begin(), type_name.end());
		string str_obj_name(obj_name.begin(), obj_name.end());

		stringstream ss;

		ss << hex << showbase;

		ss << DML_CMD << "dt " << getObjectStructByName(str_obj_name) << " " << HEX_ADDR(obj_addr + 0x30)
			<< DML_TEXT << "dt"
			<< DML_END
			<< " "
			<< noshowbase << dec << setfill(' ');

		ss << "<link cmd=\"!object ";

		ss << obj_addr + 0x30 << "\">" << setw(18) << obj_addr << "</link> "
			<< setw(20) << str_type_name << " [" << setw(4) << (uint16_t)r_index << "]   ";

		if (obj_name.empty() && type_name == L"File")
		{
			wstring file_name = dump_file_name(obj_addr + 0x30);
			string str_file_name(file_name.begin(), file_name.end());

			ss << str_file_name;
		}
		else
			ss << str_obj_name;

		ss << endl;

		if (!b_simple)
		{

			uint8_t info_mask = obj_hdr.Field("InfoMask").GetUchar();

			size_t mask2off_table_addr = EXT_F_Sym2Addr("nt!ObpInfoMaskToOffset");
			uint8_t offset = EXT_F_READ<uint8_t>(mask2off_table_addr + info_mask);

			size_t opt_hdr_start = obj_addr - offset;

			if (info_mask & 0x10)
			{
				ExtRemoteTyped ob_opt_info("(nt!_OBJECT_HEADER_PROCESS_INFO*)@$extin", opt_hdr_start);

				ss << string(25, '-') << setw(30) << "[ process info : "
					<< hex << showbase
					<< "<link cmd =\"dt nt!_OBJECT_HEADER_PROCESS_INFO "
					<< opt_hdr_start
					<< "\">"
					<< opt_hdr_start
					<< "</link>"
					<< " ]" << string(25, '-') << endl;

				size_t proc_addr = ob_opt_info.Field("ExclusiveProcess").GetUlongPtr();

				ss << "Process: <link cmd=\"!dk process " << proc_addr << "\">" << proc_addr << "</link>" << endl;

				opt_hdr_start += 0x10;
			}

			if (info_mask & 0x08)
			{
				ExtRemoteTyped ob_opt_info("(nt!_OBJECT_HEADER_QUOTA_INFO*)@$extin", opt_hdr_start);

				ss << string(25, '-') << setw(30) << "[ quota info : "
					<< hex << showbase
					<< "<link cmd =\"dt nt!_OBJECT_HEADER_QUOTA_INFO "
					<< opt_hdr_start
					<< "\">"
					<< opt_hdr_start
					<< "</link>"
					<< " ]" << string(25, '-') << endl;

				uint32_t page_charge = ob_opt_info.Field("PagedPoolCharge").GetUlong();
				uint32_t npage_charge = ob_opt_info.Field("NonPagedPoolCharge").GetUlong();
				uint32_t sd_charge = ob_opt_info.Field("SecurityDescriptorCharge").GetUlong();

				ss << setw(40) << "Paged Pool Charge: " << page_charge << endl
					<< setw(40) << "Non-Paged Pool Charge: " << npage_charge << endl
					<< setw(40) << "Security Descrptor Charge: " << sd_charge << endl;

				opt_hdr_start += 0x20;
			}

			if (info_mask & 0x04)
			{
				ExtRemoteTyped ob_opt_info("(nt!_OBJECT_HEADER_HANDLE_INFO*)@$extin", opt_hdr_start);

				ss << string(25, '-') << setw(30) << "[ handle info : "
					<< hex << showbase
					<< "<link cmd =\"dt nt!_OBJECT_HEADER_HANDLE_INFO "
					<< opt_hdr_start
					<< "\">"
					<< opt_hdr_start
					<< "</link>"
					<< " ]" << string(25, '-') << endl;

				opt_hdr_start += 0x10;
			}

			if (info_mask & 0x02)
			{
				ExtRemoteTyped ob_opt_info("(nt!_OBJECT_HEADER_NAME_INFO*)@$extin", opt_hdr_start);
				wstring obj_name = EXT_F_READ_USTR(obj_addr - offset + ob_opt_info.GetFieldOffset("Name"));

				wstring type_name = getTypeName(real_index(obj_hdr.Field("TypeIndex").GetUchar(), obj_addr));
				if (type_name == L"SymbolicLink")
				{
					obj_name += L" --> ";
					obj_name += dump_sym_link(obj_addr);
				}

				ss << string(25, '-') << setw(30) << "[ name info : "
					<< hex << showbase
					<< "<link cmd =\"dt nt!_OBJECT_HEADER_NAME_INFO "
					<< opt_hdr_start
					<< "\">"
					<< opt_hdr_start
					<< "</link>"
					<< " ]" << string(25, '-') << endl;

				size_t dir_addr = ob_opt_info.Field("Directory").GetUlongPtr();

				if (dir_addr != 0)
				{
					ss << "Parent Directory: <link cmd=\"dt nt!_OBJECT_DIRECTORY " << dir_addr << "\">" << dir_addr << "</link> "
						<< "\t\t<link cmd=\"!dk obj " << dir_addr - 0x30 << "\">detail</link> "
						<< "\t\t<link cmd=\"!dk obj_dir " << dir_addr << "\">listdir</link>"
						<< endl;
				}

				opt_hdr_start += 0x20;
			}

			if (info_mask & 0x01)
			{
				ExtRemoteTyped ob_opt_info("(nt!_OBJECT_HEADER_CREATOR_INFO*)@$extin", opt_hdr_start);

				ss << string(25, '-') << setw(30) << "[ creator info : "
					<< hex << showbase
					<< "<link cmd =\"dt nt!_OBJECT_HEADER_CREATOR_INFO "
					<< opt_hdr_start
					<< "\">"
					<< opt_hdr_start
					<< "</link>"
					<< " ]" << string(25, '-') << endl;

				size_t proc_addr = ob_opt_info.Field("CreatorUniqueProcess").GetUlongPtr();

				ss << "Creator: <link cmd=\"!dk process " << proc_addr << "\">" << proc_addr << "</link>" << endl;

				opt_hdr_start += 0x20;
			}

			ss << string(25, '-') << setw(30) << "[ security descriptor : "
				<< hex << showbase
				<< "<link cmd =\"dt nt!_SECURITY_DESCRIPTOR_RELATIVE "
				<< sdr_addr
				<< "\">"
				<< sdr_addr
				<< "</link>"
				<< " ]" << string(25, '-') << endl;
		}

		EXT_F_DML(ss.str().c_str());

		if (!b_simple && sdr_addr != 0)
			dump_sdr(sdr_addr, str_type_name);

	}
	FC;
}

void
dump_obj_dir(
	size_t obj_dir_addr,
	size_t level,
	bool recurse)
{
	if (obj_dir_addr == 0)
		return;

	try
	{
		/*ExtRemoteTyped obj_hdr("(nt!_OBJECT_HEADER*)@$extin", obj_hdr_addr);
		uint8_t info_mask = obj_hdr.Field("InfoMask").GetUchar();
		size_t obj_dir_addr = 0;
		if (info_mask & 0x02)
		{
			size_t mask2off_table_addr = getSymbolAddr("nt!ObpInfoMaskToOffset");
			uint8_t offset = read<uint8_t>(mask2off_table_addr + (info_mask & 3));

			ExtRemoteTyped ob_type_entry("(nt!_OBJECT_HEADER_NAME_INFO*)@$extin", obj_hdr_addr - offset);
			obj_dir_addr = ob_type_entry.Field("Directory").GetUlongPtr();
		}*/

		//size_t obj_dir_addr = obj_hdr_addr + 0x30;
		if (obj_dir_addr == 0)
			return;

		stringstream ss;

		dump_obj(obj_dir_addr - 0x30);

		EXT_F_OUT(string(60, '=').append("\n").c_str());

		for (size_t i = 0; i < 37; i++)
		{
			size_t entry = EXT_F_READ<size_t>(obj_dir_addr + 8 * i);
			if (!EXT_F_ValidAddr(entry))
				continue;
			if (entry == obj_dir_addr)
				continue;

			while (EXT_F_ValidAddr(entry))
			{
				size_t obj_addr = EXT_F_READ<size_t>(entry + 8);
				if (EXT_F_ValidAddr(obj_addr - 0x30))
				{
					ExtRemoteTyped obj_hdr("(nt!_OBJECT_HEADER*)@$extin", obj_addr - 0x30);
					wstring wstr_type_name = getTypeName(real_index(obj_hdr.Field("TypeIndex").GetUchar(), obj_addr - 0x30));
					string type_name(wstr_type_name.begin(), wstr_type_name.end());
					string tab(level * 4, ' ');
					tab += "->";

					ss.str("");

					ss << tab << showbase << hex << setw(4) << i
						<< " [" << setw(20) << type_name << "]     ";

					wstring wstr_file_name = dump_file_name(obj_addr - 0x30);
					wstring wstr_obj_name = dump_obj_name(obj_addr - 0x30);

					if (type_name == "File")
						ss << setw(50) << string(wstr_file_name.begin(), wstr_file_name.end());
					else
						ss << setw(50) << string(wstr_obj_name.begin(), wstr_obj_name.end());

					ss << " <link cmd=\"dt _OBJECT_HEADER " << obj_addr - 0x30 << "\">" << obj_addr - 0x30 << "</link> ";

					if (type_name == "Directory")
						ss << " <link cmd=\"!dk obj " << obj_addr - 0x30 << "\">detail</link> <link cmd=\"!dk obj_dir " << obj_addr << "\">listdir</link>" << endl;
					else
						ss << " <link cmd=\"!dk obj " << obj_addr - 0x30 << "\">detail</link>" << endl;

					EXT_F_DML(ss.str().c_str());

					/*if (type_name == "Device")
					{
						ExtRemoteTyped dev_obj("(nt!_DEVICE_OBJECT*)@$extin", obj_addr);
						size_t dev_sd = dev_obj.Field("SecurityDescriptor").GetPtr();
						dump_sdr(dev_sd);
						Out("-------------------------------------------------------------------------------\n");
					}*/

					/*if (type_name == L"File")
						Out(L"%s%02x %20s %s\n", tab.c_str(), i, type_name.c_str(), dump_file_name(obj_addr - 0x30).c_str());
					else
						Out(L"%s%02x %20s %s\n", tab.c_str(), i, type_name.c_str(), dump_obj_name(obj_addr - 0x30).c_str());
*/
//dump_obj(obj_addr-0x30, true);

/*                    if (recurse && type_name == "Directory")
						dump_obj_dir(obj_addr-0x30, level+1);     */
				}
				size_t next = EXT_F_READ<size_t>(entry);
				if (next == entry)
					break;
				entry = next;
			}
		}
	}
	FC;
}

wstring dump_obj_name(size_t obj_hdr_addr)
{
	wstring obj_name;

	try
	{
		ExtRemoteTyped obj_hdr("(nt!_OBJECT_HEADER*)@$extin", obj_hdr_addr);
		uint8_t info_mask = obj_hdr.Field("InfoMask").GetUchar();
		if (info_mask & 0x02)
		{
			size_t mask2off_table_addr = EXT_F_Sym2Addr("nt!ObpInfoMaskToOffset");
			uint8_t offset = EXT_F_READ<uint8_t>(mask2off_table_addr + (info_mask & 3));

			ExtRemoteTyped ob_type_entry("(nt!_OBJECT_HEADER_NAME_INFO*)@$extin", obj_hdr_addr - offset);
			obj_name = EXT_F_READ_USTR(obj_hdr_addr - offset + ob_type_entry.GetFieldOffset("Name"));

			wstring type_name = getTypeName(real_index(obj_hdr.Field("TypeIndex").GetUchar(), obj_hdr_addr));
			if (type_name == L"SymbolicLink")
			{
				obj_name += L" --> ";
				obj_name += dump_sym_link(obj_hdr_addr);
			}
		}
	}
	FC;

	return obj_name;
}

wstring dump_file_name(size_t file_obj_addr)
{
	try
	{
		ExtRemoteTyped file_obj("(nt!_FILE_OBJECT*)@$extin", file_obj_addr);
		return EXT_F_READ_USTR(file_obj_addr + file_obj.GetFieldOffset("FileName"));
	}
	FC;

	return L"";
}


wstring getTypeName(size_t index)
{
	if (g_type_name_map.find((uint8_t)index) == g_type_name_map.end())
	{
		size_t type_entry_addr = 0;
		if (S_OK == EXT_D_IDebugDataSpaces->ReadVirtual(g_type_index_table_addr + index * 0x08, &type_entry_addr, sizeof(size_t), NULL))
		{
			ExtRemoteTyped ob_type_entry("(nt!_OBJECT_TYPE*)@$extin", type_entry_addr);
			g_type_name_map[(uint8_t)index] = EXT_F_READ_USTR(type_entry_addr + ob_type_entry.GetFieldOffset("Name"));
		}
	}

	if (g_type_name_map.find((uint8_t)index) != g_type_name_map.end())
		return g_type_name_map[(uint8_t)index];

	return L"";
}

wstring dump_sym_link(size_t addr)
{
	wstring obj_name;

	try
	{
		ExtRemoteTyped obj_sym_link("(nt!_OBJECT_SYMBOLIC_LINK*)@$extin", addr + 0x30);
		obj_name = EXT_F_READ_USTR(addr + 0x30 + obj_sym_link.GetFieldOffset("LinkTarget"));
	}
	FC;

	return obj_name;
}
