#include "page.h"
#include "CmdExt.h"
#include "CmdList.h"
#include "process.h"

#include <iomanip>

DEFINE_CMD(page)
{
	size_t addr = EXT_F_IntArg(args, 1, 0);
	dump_page_info(addr);
}

DEFINE_CMD(pages)
{
	size_t addr = EXT_F_IntArg(args, 1, 0);
	dump_pages_around(addr);
}

DEFINE_CMD(hole)
{
	size_t rsp_addr = EXT_F_IntArg(args, 1, 0);
	dump_hole(rsp_addr);
}

//
//void CTokenExt::dump_page_info(size_t addr)
//{
//	size_t pxe_index = (addr >> 39) & 0x1FF;
//	size_t ppe_index = (addr >> 30) & 0x1FF;
//	size_t pde_index = (addr >> 21) & 0x1FF;
//	size_t pte_index = (addr >> 12) & 0x1FF;
//	size_t offset = addr & 0xFFF;
//
//	stringstream ss;
//
//	size_t cr3 = get_cr3();
//
//	PTE64 pxe(readX<size_t>(cr3 + pxe_index * 8));
//	PTE64 ppe(pxe.valid()? readX<size_t>(pxe.PFN() + ppe_index * 8) : 0);
//	PTE64 pde(ppe.valid()? readX<size_t>(ppe.PFN() + pde_index * 8) : 0);
//	PTE64 pte(pde.valid()? readX<size_t>(pde.PFN() + pte_index * 8) : 0);
//
//	size_t paddr = pte.PFN();
//
//	ss << hex << showbase
//		<< "PXE: Valid=" << (pxe.valid() ? "Y" : "n") << ", Index=" << setw(6) << pxe_index << ", PFN=" << setw(18) << pxe.PFN() << ", Flags=" << pxe.str() << endl
//		<< "PPE: Valid=" << (ppe.valid() ? "Y" : "n") << ", Index=" << setw(6) << ppe_index << ", PFN=" << setw(18) << ppe.PFN() << ", Flags=" << ppe.str() << endl
//		<< "PDE: Valid=" << (pde.valid() ? "Y" : "n") << ", Index=" << setw(6) << pde_index << ", PFN=" << setw(18) << pde.PFN() << ", Flags=" << pde.str() << endl
//		<< "PTE: Valid=" << (pte.valid() ? "Y" : "n") << ", Index=" << setw(6) << pte_index << ", PFN=" << setw(18) << pte.PFN() << ", Flags=" << pte.str() << endl
//		<< "Virtual Address: " << setw(18) << addr << endl
//		<< "Physical Address: " << setw(18) << (pte.PFN() + offset) << endl << endl;
//
//	Out(ss.str().c_str());
//}

void dump_page_info(size_t addr)
{
	size_t pxe_index = (addr >> 39) & 0x1FF;
	size_t ppe_index = (addr >> 30) & 0x1FF;
	size_t pde_index = (addr >> 21) & 0x1FF;
	size_t pte_index = (addr >> 12) & 0x1FF;
	size_t offset = addr & 0xFFF;

	stringstream ss;

	size_t cr3 = get_cr3() & 0xFFFFFFFFFFFFFFF0;

	PTE64 pxe(EXT_F_READX<size_t>(cr3 + pxe_index * 8));
	PTE64 ppe(pxe.valid() ? EXT_F_READX<size_t>(pxe.PFN() + ppe_index * 8) : 0);
	PTE64 pde(ppe.valid() ? EXT_F_READX<size_t>(ppe.PFN() + pde_index * 8) : 0);
	PTE64 pte(pde.valid() ? EXT_F_READX<size_t>(pde.PFN() + pte_index * 8) : 0);

	size_t paddr = pte.PFN();

	ss << hex << showbase
		<< "PXE: Valid=" << (pxe.valid() ? "Y" : "n") << ", Index=" << setw(6) << pxe_index << ", PFN=" << setw(18) << pxe.PFN() / 0x1000 << ", Flags=" << pxe.str() << endl
		<< "PPE: Valid=" << (ppe.valid() ? "Y" : "n") << ", Index=" << setw(6) << ppe_index << ", PFN=" << setw(18) << ppe.PFN() / 0x1000 << ", Flags=" << ppe.str() << endl
		<< "PDE: Valid=" << (pde.valid() ? "Y" : "n") << ", Index=" << setw(6) << pde_index << ", PFN=" << setw(18) << pde.PFN() / 0x1000 << ", Flags=" << pde.str() << endl
		<< "PTE: Valid=" << (pte.valid() ? "Y" : "n") << ", Index=" << setw(6) << pte_index << ", PFN=" << setw(18) << pte.PFN() / 0x1000 << ", Flags=" << pte.str() << endl
		<< "Virtual Address: " << setw(18) << addr << endl
		<< "Physical Address: " << setw(18) << (pte.PFN() + offset) << endl << endl;

	EXT_F_OUT(ss.str().c_str());
}

void dump_pages_around(size_t addr)
{
	size_t pxe_index = (addr >> 39) & 0x1FF;
	size_t ppe_index = (addr >> 30) & 0x1FF;
	size_t pde_index = (addr >> 21) & 0x1FF;
	size_t pte_index = (addr >> 12) & 0x1FF;
	size_t offset = addr & 0xFFF;

	stringstream ss;

	size_t cr3 = get_cr3() & 0xFFFFFFFFFFFFFFF0;

	PTE64 pxe(EXT_F_READX<size_t>(cr3 + pxe_index * 8));
	PTE64 ppe(pxe.valid() ? EXT_F_READX<size_t>(pxe.PFN() + ppe_index * 8) : 0);
	PTE64 pde(ppe.valid() ? EXT_F_READX<size_t>(ppe.PFN() + pde_index * 8) : 0);

	size_t* pde_entries = new size_t[0x200];
	memset(pde_entries, 0, 0x1000);

	if (pxe.valid() && ppe.valid() && pde.valid())
	{
		auto status = EXT_D_IDebugDataSpaces->ReadPhysical(pde.PFN(), pde_entries, 0x1000, NULL);
		if (S_OK != status)
			EXT_F_THROW(E_ACCESSDENIED, "Fail to read memory");
	}

	for (size_t i = 0; i < 0x200; i++)
	{
		PTE64 pte(pde_entries[i]);

		ss << hex << showbase
			<< setw(18) << ((addr & 0xFFFFFFFFFFE00000) + i * 0x1000) << " : ";

		if (pte.valid())
			ss << setw(18) << pte.PFN();

		if (i == pte_index)
			ss << "\t\t<---------------- [" << addr << " ]";

		ss << endl;
	}

	delete[] pde_entries;

	EXT_F_DML("<link cmd=\"!dk pages 0x%I64x\"> Previous </link>\n", addr - 0x200000);
	EXT_F_OUT(ss.str().c_str());
	EXT_F_DML("<link cmd=\"!dk pages 0x%I64x\"> Next </link>\n", addr + 0x200000);
}

void dump_hole(size_t addr)
{
	try
	{
		//if (valid_addr(addr))
		bool bHoleFound = false;
		while (!bHoleFound)
		{
			bool current = false;

			size_t start_page_addr = (addr & 0xFFFFFFFFFFFFF000 - 0x1000 * 0x80) & 0xFFFFFFFFFFF00000;

			stringstream ss;
			ss << "\n++++++++++++++++++++++++++++++++++++++++++++++\n";
			for (size_t i = 0; i < 0x200; i++)
			{
				if (i % 0x10 == 0)
				{
					if (current)
					{
						current = false;
						ss << "\t\t <-----[" << hex << showbase << setw(16) << setfill('0') << "]" << addr;
					}
					ss << "\n" << hex << showbase << setw(16) << setfill('0') << start_page_addr + i * 0x1000 << ": ";
				}

				if (addr >= start_page_addr + i * 0x1000 && addr <= start_page_addr + (i + 1) * 0x1000)
					current = true;

				if (EXT_F_ValidAddr(start_page_addr + i * 0x1000))
					ss << "!";
				else
				{
					ss << ".";
					bHoleFound = true;
				}
			}

			ss << "\n++++++++++++++++++++++++++++++++++++++++++++++\n";

			EXT_F_OUT(ss.str().c_str());

			addr += 0x200000;
		}
	}
	FC;
}