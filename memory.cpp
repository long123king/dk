#include "model.h"
#include "memory.h"
#include "CmdExt.h"
#include "CmdList.h"
#include "process.h"
#include "object.h"
#include "page.h"
#include "pool.h"
#include <iomanip>
#include <set>

#include "svg/Coordinates.h"
#include "svg/SvgElements.h"

#pragma warning( disable : 4244)

set<tuple<size_t, size_t, string>> g_va_regions;

DEFINE_CMD(size)
{
    size_t value = EXT_F_IntArg(args, 1, 0);
    dump_size(value);
}

DEFINE_CMD(va_regions)
{
    dump_va_regions();
}

DEFINE_CMD(regs)
{
    dump_regs();
}

DEFINE_CMD(as_qword)
{
    size_t value = EXT_F_IntArg(args, 1, 0);
    analyze_qword(value);
}

DEFINE_CMD(as_mem)
{
    size_t addr = EXT_F_IntArg(args, 1, 0);
    size_t len = EXT_F_IntArg(args, 2, 0);

    analyze_mem(addr, len);
}

DEFINE_CMD(ex_mem)
{
    size_t addr = EXT_F_IntArg(args, 1, 0);
    size_t len = EXT_F_IntArg(args, 2, 0);

    extract_mem(addr, len);
}

DEFINE_CMD(args)
{
    dump_args();
}

DEFINE_CMD(vad)
{
    size_t root_addr = EXT_F_IntArg(args, 1, 0);
    dump_vad(root_addr);
}

DEFINE_CMD(memcpy)
{
    size_t src_addr = EXT_F_IntArg(args, 1, 0);
    size_t dst_addr = EXT_F_IntArg(args, 2, 0);
    size_t count = EXT_F_IntArg(args, 3, 0);
    do_memcpy(src_addr, dst_addr, count);
}

DEFINE_CMD(page_2_svg)
{
    if (args.size() < 3)
    {
        EXT_F_OUT("Usage: !dk page_2_svg <addr> <output_filename>\n");
        return;
    }
    size_t addr = EXT_F_IntArg(args, 1, 0);
    string out_filename = args[2];

    page_to_svg(addr, out_filename);
}

void dump_regs()
{
    static vector<const char*> regs_name{
        "rip", "rsp",
        "rax", "rbx", "rcx", "rdx",
        "rbp", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };

    try
    {
        stringstream ss;
        for (auto& reg_entry : regs_name)
        {
            size_t reg_value = EXT_F_REG_OF(reg_entry);
            ss.str("");
            ss << "                " << reg_entry << ": \t" << hex << showbase << setw(18) << reg_value << " ";
            EXT_F_OUT(ss.str().c_str());
            analyze_qword(reg_value);
        }
    }
    FC;
}

#define GRID_WIDTH          40
#define GRID_HEIGHT         30
#define GRID_ADDR_WIDTH     180

string Byte2FontStyle(uint8_t byte)
{
    string character_color = "#affc41";
    string numeric_color = "#16db93";
    string software_breakpoing_color = "#0466c8";
    string maximum_color = "#a100f2";
    string non_ascii = "#ff0708";
    string ascii_default = "#ffd60a";

    string fill_color = ascii_default;
    if (byte == 0)
        return "";
    else if (byte >= 'a' && byte <= 'z')
        fill_color = character_color;
    else if (byte >= 'A' && byte <= 'Z')
        fill_color = character_color;
    else if (byte >= '0' && byte <= '9')
        fill_color = numeric_color;
    else if (byte == 0xCC)
        fill_color = software_breakpoing_color;
    else if (byte == 0xFF)
        fill_color = maximum_color;
    else if (byte >= 0x80)
        fill_color = non_ascii;

    return "fill: " + fill_color + ";";
}

shared_ptr<CSvgDoc> visual_page(string page_content, uint64_t page_addr, CoordinatesManager& coordinates_mgr)
{
    auto canvas_size = coordinates_mgr.GetCanvasSize();
    auto svg_doc = make_shared<CSvgDoc>(get<0>(canvas_size), get<1>(canvas_size), CSvgPoint(0, 0), get<0>(canvas_size), get<1>(canvas_size));

    auto svg_def = make_shared<CSvgDefsArrowHead>("arrowhead", 10, 7);
    svg_def->addStyle("stroke: none; fill:red; fill-opacity: 0.4; stroke-opacity: 0.4;");

    vector<string> addr_texts;
    vector<string> addr_styles;
    for (uint64_t i = 0; i < 0x200; i++)
    {
        stringstream ss;
        ss << "0x" << hex << setfill('0') << setw(16) << page_addr + (i * 8);

        string text = ss.str();
        text.insert(10, "`");
        addr_texts.push_back(text);
    }
    auto addr_pivot = coordinates_mgr.GetLAddrPivot(0);
    auto addr_parameters = coordinates_mgr.GetLAddrParameters();
    auto svg_addr_grids = make_shared<CSvgGrids>(CSvgPoint(get<0>(addr_pivot), get<1>(addr_pivot)),
        get<0>(addr_parameters), get<1>(addr_parameters), get<2>(addr_parameters), get<3>(addr_parameters),
        addr_texts, vector<string>(), vector<string>());

    svg_addr_grids->addLineStyle("stroke: white;");
    svg_addr_grids->addTextStyle("stroke: none; fill: black; font-size: 16; font-weight: normal; font-family: monospace; ");
    svg_addr_grids->addRectStyle("stroke: none; fill: none; font-size: 16; font-weight: normal;");

    vector<string> content_texts;
    vector<string> content_styles;
    for (uint64_t i = 0; i < 0x1000; i++)
    {
        stringstream ss;
        ss << hex << uppercase << setfill('0') << setw(2) << (uint16_t)(page_content[i] & 0x00FF);
        content_texts.push_back(ss.str());

        ss.str("");
        ss << Byte2FontStyle((uint8_t)page_content[i]);
        content_styles.push_back(ss.str());
    }

    auto grid_pivot = coordinates_mgr.GetLGridPivot(0, 0);
    auto grid_parameters = coordinates_mgr.GetLGridParameters();
    auto svg_content_grids = make_shared<CSvgGrids>(CSvgPoint(get<0>(grid_pivot), get<1>(grid_pivot)),
        get<0>(grid_parameters), get<1>(grid_parameters), get<2>(grid_parameters), get<3>(grid_parameters),
        content_texts, content_styles, vector<string>());

    svg_content_grids->addLineStyle("stroke: white; fill: white;");
    svg_content_grids->addTextStyle("font-family: sans-serif; fill-width: 6; stroke: none; fill: white; font-size: 16; font-weight: normal;");
    svg_content_grids->addRectStyle("stroke: white; stroke-width: 2; fill: black; font-size: 16; font-weight: normal;");

    svg_doc->appendElement(svg_def);
    svg_doc->appendElement(svg_addr_grids);
    svg_doc->appendDynamicElement(svg_content_grids);

    return svg_doc;
}

void update_page(shared_ptr<CSvgDoc> svg_doc, string page_content, CoordinatesManager& coordinates_mgr)
{
    svg_doc->resetDynamicElements();

    vector<string> content_texts;
    vector<string> content_styles;
    for (uint64_t i = 0; i < 0x1000; i++)
    {
        stringstream ss;
        ss << hex << uppercase << setfill('0') << setw(2) << (uint16_t)(page_content[i] & 0x00FF);
        content_texts.push_back(ss.str());

        ss.str("");
        ss << Byte2FontStyle((uint8_t)page_content[i]);
        content_styles.push_back(ss.str());
    }

    auto grid_pivot = coordinates_mgr.GetLGridPivot(0, 0);
    auto grid_parameters = coordinates_mgr.GetLGridParameters();

    auto svg_content_grids = make_shared<CSvgGrids>(CSvgPoint(get<0>(grid_pivot), get<1>(grid_pivot)),
        get<0>(grid_parameters), get<1>(grid_parameters), get<2>(grid_parameters), get<3>(grid_parameters),
        content_texts, content_styles, vector<string>());

    svg_content_grids->addLineStyle("stroke: white; fill: white;");
    svg_content_grids->addTextStyle("font-family: sans-serif; fill-width: 6; stroke: none; fill: white; font-size: 16; font-weight: normal;");
    svg_content_grids->addRectStyle("stroke: white; stroke-width: 2; fill: black; font-size: 16; font-weight: normal;");

    svg_doc->appendDynamicElement(svg_content_grids);
}

void AddStackVariable(shared_ptr<CSvgGroup> g_arrows, shared_ptr<CSvgGroup> g_texts, uint64_t variable_addr, string variable_name, CoordinatesManager& coordinates_mgr)
{
    uint64_t v_page = variable_addr & 0xFFFFFFFFFFFFF000;

    uint64_t v_row_index = ((variable_addr - v_page) + 8) / 8;
    uint64_t v_column_index = variable_addr % 8;

    auto v_pivot = coordinates_mgr.GetMRowPivot(v_row_index);
    auto v_target_pivot = coordinates_mgr.GetLGridPivot(v_column_index + 1, v_row_index);

    auto arrow_v = make_shared<CSvgArrow>(
        CSvgPoint(get<0>(v_pivot), get<1>(v_pivot) - get<1>(coordinates_mgr.GetLAddrParameters()) / 2),
        CSvgPoint(get<0>(v_target_pivot), get<1>(v_target_pivot) - get<1>(coordinates_mgr.GetLAddrParameters()) / 2),
        "arrowhead");

    stringstream ss;
    ss << "stack variable: " << variable_name << "; ( " << hex << showbase << (uint64_t)variable_addr << " )";
    auto text_v = make_shared<CSvgText>(CSvgPoint(get<0>(v_pivot) + 10, get<1>(v_pivot) - get<1>(coordinates_mgr.GetLAddrParameters()) / 2), ss.str());

    g_arrows->appendElement(arrow_v);
    g_texts->appendElement(text_v);
}

void page_to_svg(size_t addr, string svg_filename)
{
    string page(0x1000, '0');

    size_t ch_page = addr & 0xFFFFFFFFFFFFF000;

    size_t bytes_read = 0;
    HRESULT result = EXT_D_IDebugDataSpaces->ReadVirtual(ch_page, (uint8_t*)page.data(), 0x1000, (PULONG)&bytes_read);
    if (result != 0)
    {
        EXT_F_ERR("Fail to read memory in [ 0x%0I64x - 0x%0I64x ], HRESULT: 0x%08x\n", ch_page, ch_page + 0x1000, result);
        return;
    }

    CoordinatesManager coordinates_mgr(GRID_WIDTH, GRID_HEIGHT, GRID_ADDR_WIDTH);

    auto stack_svg_doc = visual_page(page, (uint64_t)ch_page, coordinates_mgr);

    auto svg_g_arrows = make_shared<CSvgGroup>();
    svg_g_arrows->addStyle("stroke: red; stroke-width: 3; stroke-opacity: 0.4;");

    auto svg_g_texts = make_shared<CSvgGroup>();
    svg_g_texts->addStyle("stroke: none; fill: black; font-size: 16; font-weight: normal; font-family: monospace; ");

    AddStackVariable(svg_g_arrows, svg_g_texts, (uint64_t)addr, "addr", coordinates_mgr);

    stack_svg_doc->appendDynamicElement(svg_g_arrows);
    stack_svg_doc->appendDynamicElement(svg_g_texts);

    stack_svg_doc->Save(svg_filename);
}

void dump_size(size_t value)
{
    try
    {
        size_t k_size = value / 1024;
        size_t m_size = k_size / 1024;
        size_t g_size = m_size / 1024;
        size_t t_size = g_size / 1024;

        stringstream ss;

        ss << showbase << hex;

        if (t_size != 0)
            ss << setw(6) << t_size << " T ";

        if (g_size != 0)
            ss << setw(6) << g_size - t_size * 1024 << " G ";

        if (m_size != 0)
            ss << setw(6) << m_size - g_size * 1024 << " M ";

        if (k_size != 0)
            ss << setw(6) << k_size - m_size * 1024 << " K ";

        ss << value - k_size * 1024;

        ss << endl;
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

        ss << endl;

        EXT_F_OUT(ss.str().c_str());
    }
    FC;
}

void dump_args()
{
    try
    {
        size_t rcx = EXT_F_REG_OF("rcx");
        size_t rdx = EXT_F_REG_OF("rdx");
        size_t r8 = EXT_F_REG_OF("r8");
        size_t r9 = EXT_F_REG_OF("r9");

        size_t rsp = EXT_F_REG_OF("rsp");

        stringstream ss;

        ss << "                rcx: \t" << hex << showbase << setw(18) << rcx << " ";
        EXT_F_OUT(ss.str().c_str());
        analyze_qword(rcx);

        ss.str("");
        ss << "                rdx: \t" << hex << showbase << setw(18) << rdx << " ";
        EXT_F_OUT(ss.str().c_str());
        analyze_qword(rdx);

        ss.str("");
        ss << "                 r8: \t" << hex << showbase << setw(18) << r8 << " ";
        EXT_F_OUT(ss.str().c_str());
        analyze_qword(r8);

        ss.str("");
        ss << "                 r9: \t" << hex << showbase << setw(18) << r9 << " ";
        EXT_F_OUT(ss.str().c_str());
        analyze_qword(r9);

        EXT_F_OUT("------------------------------ [ stacks : 0x%I64x ] --------------------------------\n", rsp);
        analyze_mem(rsp, 0x80);
    }
    FC;
}

void analyze_qword(size_t value)
{
    try
    {
        stringstream ss;

        //ss << "Analyze Qword : " << hex << showbase << value << endl;

        if (like_kaddr(value))
        {
            size_t curr_qword = value;
            string region_name = va_region_name(value);
            bool is_paged_pool = in_paged_pool(curr_qword);
            bool is_nonpaged_pool = in_non_paged_pool(curr_qword);

            if (in_curr_stack(curr_qword))
            {
                ss << setw(18) << "[ stack ] " << "rsp+" << showbase << hex << (curr_qword - EXT_F_REG_OF("rsp")) << ": "
                    << "\t<link cmd=\"db " << value << "\">db</link>"
                    << "\t<link cmd=\"dq " << value << "\">dq</link>"
                    << "\t<link cmd=\"dps " << value << "\">dps</link>"
                    << endl;
            }
            else if (is_paged_pool || is_nonpaged_pool)
            {
                bool is_small_pool = in_small_pool_page(curr_qword);

                auto pool_entry = is_small_pool ? as_small_pool(curr_qword) : as_large_pool(curr_qword);

                if (get<4>(pool_entry) == 0 && get<3>(pool_entry) == 0)
                {
                    ss << setw(18) << " [ pool ] "
                        << dump_plain_qword(curr_qword)
                        << (is_paged_pool ? " paged " : " non-paged ")
                        << showbase << hex << "<link cmd=\"!pool " << curr_qword << "\">" << curr_qword << "</link>" << endl;
                }
                else
                {
                    ss << setw(18) << " [ pool ] "
                        << dump_plain_qword(curr_qword)
                        << " " <</*" <link cmd=\"!pooltag " << get<5>(pool_entry) << ";\">" <<*/ setw(4) << get<5>(pool_entry) << /*"</link>*/"\t\t"
                        << "<link cmd=\"!dk as_mem " << hex << showbase << get<3>(pool_entry) << " " << get<4>(pool_entry) << ";\">["
                        << get<3>(pool_entry) << ",  "
                        << setw(10) << (curr_qword - get<3>(pool_entry)) << " / "
                        << setw(10) << get<4>(pool_entry) << "]</link>\t\t";

                    if (get<0>(pool_entry))
                        ss << "Small | ";
                    else
                        ss << "Large | ";

                    if (get<1>(pool_entry))
                        ss << "Paged | ";
                    else
                        ss << "Non-Paged | ";

                    if (get<2>(pool_entry))
                        ss << "Allocated";
                    else
                        ss << "Free";

                    ss << endl;
                }
            }
            else if (region_name != "" && region_name.length() > 14)
            {
                ss << setw(18) << " [ region ] "
                    << dump_plain_qword(curr_qword)
                    << " " << region_name.substr(14)
                    << endl;
            }
            else
            {
                auto code_details = as_kcode(curr_qword);

                if (get<0>(code_details))
                {
                    ss << setw(18) << " [ code ] " << dump_plain_qword(curr_qword);


                    ss << hex << showbase << "\t<link cmd=\"u " << curr_qword << ";\">disasm</link>\t";

                    if (get<3>(code_details).empty())
                        ss << get<2>(code_details) << ":" << (curr_qword - get<1>(code_details)) << endl;
                    else
                        ss << get<3>(code_details) << endl;
                }
                else
                {
                    size_t pxe_index = (curr_qword >> 39) & 0x1FF;
                    size_t ppe_index = (curr_qword >> 30) & 0x1FF;
                    size_t pde_index = (curr_qword >> 21) & 0x1FF;
                    size_t pte_index = (curr_qword >> 12) & 0x1FF;
                    size_t offset = curr_qword & 0xFFF;

                    size_t cr3 = get_cr3() & 0xFFFFFFFFFFFFFFF0;

                    PTE64 pxe(EXT_F_READX<size_t>(cr3 + pxe_index * 8));
                    PTE64 ppe(pxe.valid() ? EXT_F_READX<size_t>(pxe.PFN() + ppe_index * 8) : 0);
                    PTE64 pde(ppe.valid() ? EXT_F_READX<size_t>(ppe.PFN() + pde_index * 8) : 0);
                    PTE64 pte(pde.valid() ? EXT_F_READX<size_t>(pde.PFN() + pte_index * 8) : 0);

                    size_t paddr = pte.PFN();

                    bool b_valid_page = pxe.valid() && ppe.valid() && pde.valid() && pte.valid();

                    if (b_valid_page)
                    {
                        ss << hex << showbase
                            << setw(18) << " [ valid ] "
                            << dump_plain_qword(curr_qword)
                            << setw(18) << " paddr: <link cmd=\"!db " << paddr << "\">" << paddr << "</link>\t"
                            << setw(18) << " page: <link cmd=\"!dk page " << curr_qword << "\">" << curr_qword << "</link>\t"
                            << setw(18) << " pte: <link cmd=\"!pte " << curr_qword << "\">" << curr_qword << "</link>\t"
                            /*<< pte.str()*/ << endl;
                    }
                    else
                        ss << setw(18) << " [     ] "
                        << dump_plain_qword(curr_qword)
                        << endl;
                }
            }

            EXT_F_DML(ss.str().c_str());
        }
        else
        {
            size_t curr_qword = value;

            ss << setw(18) << " [     ] "
                << dump_plain_qword(curr_qword)
                << endl;

            EXT_F_DML(ss.str().c_str());

            //{				
            //	string raw((char*)&curr_qword, 8);
            //	for (char& ch : raw)
            //	{
            //		if (is_in_range<char>(ch, 'A', 'Z') || is_in_range<char>(ch, 'a', 'z') || is_in_range<char>(ch, '0', '9'))
            //		{
            //		}
            //		else
            //		{
            //			ch = '.';
            //		}
            //	}

            //	ss << setw(18) << " [     ] "
            //		<< hex << noshowbase << setfill('0')
            //		<< "[ " << setw(8) << (curr_qword & 0xFFFFFFFF) << " " << setw(8) << ((curr_qword >> 0x20) & 0xFFFFFFFF) << " ]\t"
            //		<< "[ " << setw(2) << ((curr_qword >> 0) & 0xFF) << " " << setw(2) << ((curr_qword >> 8) & 0xFF)
            //		<< " " << setw(2) << ((curr_qword >> 16) & 0xFF) << " " << setw(2) << ((curr_qword >> 24) & 0xFF)
            //		<< " " << setw(2) << ((curr_qword >> 32) & 0xFF) << " " << setw(2) << ((curr_qword >> 40) & 0xFF)
            //		<< " " << setw(2) << ((curr_qword >> 48) & 0xFF) << " " << setw(2) << ((curr_qword >> 56) & 0xFF)
            //		<< " |" << raw << "| "
            //		<< " ]" << setfill(' ')
            //		<< endl;
            //}

            //Dml(ss.str().c_str());
        }
    }
    FC;
}

#define ZERO_REPEAT_THREADHOLD 4

void analyze_mem(size_t start, size_t len, size_t offset)
{
    while (len > 0x10000)
    {
        analyze_mem(start, 0x10000, offset);

        start += 0x10000;
        offset += 0x10000;
        len -= 0x10000;
    };

    try
    {
        uint8_t* buffer = new uint8_t[len];

        EXT_F_OUT("Analyzing memory in [ 0x%0I64x - 0x%0I64x ]\n", start, start + len);

        size_t bytes_read = 0;
        HRESULT result = EXT_D_IDebugDataSpaces->ReadVirtual(start, buffer, (ULONG)len, (PULONG)&bytes_read);
        if (result != 0)
        {
            EXT_F_ERR("Fail to read memory in [ 0x%0I64x - 0x%0I64x ], HRESULT: 0x%08x\n", start, start + len, result);
            return;
        }

        stringstream ss;
        //size_t zero_repeats = 0;


        for (size_t i = 0; i < len / 8; i++)
        {
            ss.str("");
            size_t curr_qword = *(size_t*)(buffer + i * 8);

            size_t j = i + 1;

            if (curr_qword == 0)
            {
                while (j < len / 8 && *(size_t*)(buffer + j * 8) == 0)
                {
                    j++;
                };

                if (j - i >= ZERO_REPEAT_THREADHOLD)
                {
                    ss << hex << showbase << "+" << setw(18) << offset + i * 8 << ": \t" << setw(18) << curr_qword << " ";

                    ss << setw(18) << " [ 000 ] " << dump_plain_qword(curr_qword);

                    ss << " Zeros in range [ " << hex << showbase << offset + i * 8 << " - "
                        << offset + j * 8 << " )";

                    ss << " for " <<
                        (j - i) << " / " << showbase << dec << (j - i) << " times" <<
                        endl << "." << endl;

                    EXT_F_DML(ss.str().c_str());

                    i = j;

                    if (i >= len / 8)
                        break;

                    curr_qword = *(size_t*)(buffer + i * 8);
                    ss.str("");
                }
            }
            //else
            //{
            //    //if (zero_repeats > ZERO_REPEAT_THREADHOLD) // [4-...) repeated zeros simplified as a single line
            //    //{
            //    //    ss << hex << showbase << "+" << setw(18) << offset + (len / 8 - zero_repeats + 4) * 8 <<
            //    //        " - " << offset + (len / 8 - 1) * 8 <<
            //    //        ": \t\t\t\t\t\t repeated zeros ( 0x00000000`00000000 ) * " <<
            //    //        (zero_repeats - 4) << " / 0n" << dec << (zero_repeats - 4) <<
            //    //        endl;

            //    //    Dml(ss.str().c_str());

            //    //    zero_repeats = 0;

            //    //    continue;
            //    //}

            //    zero_repeats = 0;
            //}

            ss << hex << showbase << "+" << setw(18) << offset + i * 8 << ": \t" << setw(18) << curr_qword << " ";
            if (!like_kaddr(curr_qword))
            {
                ss << setw(18) << " [     ] "
                    << dump_plain_qword(curr_qword)
                    << endl;

                EXT_F_DML(ss.str().c_str());
            }
            else if (curr_qword >= start && curr_qword < start + len)
            {
                ss << setw(18) << " [ this ] ";

                ss << dump_plain_qword(curr_qword);

                if (curr_qword == (start + i * 8))
                {
                    size_t next_qword = *(size_t*)(buffer + i * 8 + 8);

                    if (next_qword == curr_qword)
                    {
                        ss << " _LIST_ENTRY.Flink(empty)" << endl;

                        ss << hex << showbase << "+" << setw(18) << i * 8 << ": \t" << setw(18) << curr_qword << " "
                            << setw(18) << " [ this ] " << dump_plain_qword(curr_qword) << " _LIST_ENTRY.Blink(empty)" << endl;

                        i++;
                    }
                    else
                    {
                        ss << " ===> +" << (curr_qword - start) << endl;
                    }
                }
                else
                {
                    ss << " ===> +" << (curr_qword - start) << endl;
                }

                EXT_F_DML(ss.str().c_str());
            }
            else
            {
                EXT_F_DML(ss.str().c_str());
                analyze_qword(curr_qword);
            }
        }

        //if (zero_repeats > ZERO_REPEAT_THREADHOLD) // [4-...) repeated zeros simplified as a single line
        //{
        //    ss << hex << showbase << "+" << setw(18) << offset + (len/8 - zero_repeats + 4) * 8 <<
        //        " - " << offset + (len/8 - 1) * 8 <<
        //        ": \t\t\t\t\t\t repeated zeros ( 0x00000000`00000000 ) * " <<
        //        (zero_repeats - 4) << " / 0n" << dec << (zero_repeats - 4) <<
        //        endl;

        //    Dml(ss.str().c_str());

        //    zero_repeats = 0;
        //}

    }
    FC;
}

void extract_mem(size_t start, size_t len, size_t offset)
{
    while (len > 0x10000)
    {
        extract_mem(start, 0x10000, offset);

        start += 0x10000;
        offset += 0x10000;
        len -= 0x10000;
    };

    try
    {
        uint8_t* buffer = new uint8_t[len];

        EXT_F_OUT("//Analyzing memory in [ 0x%0I64x - 0x%0I64x ]\n", start, start + len);

        size_t bytes_read = 0;
        HRESULT result = EXT_D_IDebugDataSpaces->ReadVirtual(start, buffer, (ULONG)len, (PULONG)&bytes_read);
        if (result != 0)
        {
            EXT_F_ERR("//Fail to read memory in [ 0x%0I64x - 0x%0I64x ], HRESULT: 0x%08x\n", start, start + len, result);
            return;
        }

        stringstream ss;
        //size_t zero_repeats = 0;


        for (size_t i = 0; i < len / 8; i++)
        {
            ss.str("");
            size_t curr_qword = *(size_t*)(buffer + i * 8);

            size_t j = i + 1;

            if (curr_qword == 0)
            {
                while (j < len / 8 && *(size_t*)(buffer + j * 8) == 0)
                {
                    j++;
                };

                if (j - i >= ZERO_REPEAT_THREADHOLD)
                {
                    //ss << hex << showbase << "+" << setw(18) << offset + i * 8 << ": \t" << setw(18) << curr_qword << " ";

                    //ss << setw(18) << " [ 000 ] " << dump_plain_qword(curr_qword);

                    //ss << " Zeros in range [ " << hex << showbase << offset + i * 8 << " - "
                    //    << offset + j * 8 << " )";

                    //ss << " for " <<
                    //    (j - i) << " / " << showbase << dec << (j - i) << " times" <<
                    //    endl << "." << endl;

                    ss << "REP_ADD_QWORD( " << hex << showbase << (j - i) << " , " << curr_qword << " );" << endl;

                    EXT_F_DML(ss.str().c_str());

                    i = j;

                    if (i >= len / 8)
                        break;

                    curr_qword = *(size_t*)(buffer + i * 8);
                    ss.str("");
                }
            }
            //else
            //{
            //    //if (zero_repeats > ZERO_REPEAT_THREADHOLD) // [4-...) repeated zeros simplified as a single line
            //    //{
            //    //    ss << hex << showbase << "+" << setw(18) << offset + (len / 8 - zero_repeats + 4) * 8 <<
            //    //        " - " << offset + (len / 8 - 1) * 8 <<
            //    //        ": \t\t\t\t\t\t repeated zeros ( 0x00000000`00000000 ) * " <<
            //    //        (zero_repeats - 4) << " / 0n" << dec << (zero_repeats - 4) <<
            //    //        endl;

            //    //    Dml(ss.str().c_str());

            //    //    zero_repeats = 0;

            //    //    continue;
            //    //}

            //    zero_repeats = 0;
            //}

            ss << "ADD_QWORD( " << hex << showbase << curr_qword << " );" << endl;
            EXT_F_DML(ss.str().c_str());

            /*ss << hex << showbase << "+" << setw(18) << offset + i * 8 << ": \t" << setw(18) << curr_qword << " ";
            if (!like_kaddr(curr_qword))
            {
                ss << setw(18) << " [     ] "
                    << dump_plain_qword(curr_qword)
                    << endl;

                Dml(ss.str().c_str());
            }
            else if (curr_qword >= start && curr_qword < start + len)
            {
                ss << setw(18) << " [ this ] ";

                ss << dump_plain_qword(curr_qword);

                if (curr_qword == (start + i * 8))
                {
                    size_t next_qword = *(size_t*)(buffer + i * 8 + 8);

                    if (next_qword == curr_qword)
                    {
                        ss << " _LIST_ENTRY.Flink(empty)" << endl;

                        ss << hex << showbase << "+" << setw(18) << i * 8 << ": \t" << setw(18) << curr_qword << " "
                            << setw(18) << " [ this ] " << dump_plain_qword(curr_qword) << " _LIST_ENTRY.Blink(empty)" << endl;

                        i++;
                    }
                    else
                    {
                        ss << " ===> +" << (curr_qword - start) << endl;
                    }
                }
                else
                {
                    ss << " ===> +" << (curr_qword - start) << endl;
                }

                Dml(ss.str().c_str());
            }
            else
            {
                Dml(ss.str().c_str());
                analyze_qword(curr_qword);
            }*/
        }

        //if (zero_repeats > ZERO_REPEAT_THREADHOLD) // [4-...) repeated zeros simplified as a single line
        //{
        //    ss << hex << showbase << "+" << setw(18) << offset + (len/8 - zero_repeats + 4) * 8 <<
        //        " - " << offset + (len/8 - 1) * 8 <<
        //        ": \t\t\t\t\t\t repeated zeros ( 0x00000000`00000000 ) * " <<
        //        (zero_repeats - 4) << " / 0n" << dec << (zero_repeats - 4) <<
        //        endl;

        //    Dml(ss.str().c_str());

        //    zero_repeats = 0;
        //}

    }
    FC;
}


bool like_kaddr(size_t addr)
{

    if ((addr & 0xFFFF000000000000) == 0xFFFF000000000000)
    {
        if (addr == 0xffffffff00000000)
            return false;

        if (addr == 0xffffffffffffffff)
            return false;

        if ((addr & 0xffffffff) < 0x1000)
            return false;


        return true;
    }

    return false;
}

bool in_curr_stack(size_t addr)
{
    size_t rsp = EXT_F_REG_OF("rsp");

    return (addr ^ rsp) < 0x4000;
}

void init_va_regions()
{
    static vector<const char*> va_region_names{
       "AssignedRegionNonPagedPool", // 0n0
       "AssignedRegionPagedPool", // 0n1
       "AssignedRegionSystemCache", // 0n2
       "AssignedRegionSystemPtes", // 0n3
       "AssignedRegionUltraZero", // 0n4
       "AssignedRegionPfnDatabase", // 0n5
       "AssignedRegionCfg", // 0n6
       "AssignedRegionHyperSpace", // 0n7
       "AssignedRegionKernelStacks", // 0n8
       "AssignedRegionPageTables", // 0n9
       "AssignedRegionSession", // 0n10
       "AssignedRegionSecureNonPagedPool", // 0n11
       "AssignedRegionSystemImages", // 0n12
       "AssignedRegionMaximum", // 0n13
    };

    try
    {
        size_t mi_state_addr = EXT_F_Sym2Addr("nt!MiState");
        ExtRemoteTyped mi_state("(nt!_MI_SYSTEM_INFORMATION*)@$extin", mi_state_addr);

        auto va_arr = mi_state_addr;
        va_arr += mi_state.GetFieldOffset("Vs.SystemVaRegions");

        uint8_t arr_raw[0x130] = { 0, };
        size_t bytes_read = 0;
        EXT_D_IDebugDataSpaces->ReadVirtual(va_arr, arr_raw, 0x130, (PULONG)&bytes_read);

        for (size_t i = 0; i < 13; i++)
        {
            size_t addr = *(size_t*)(arr_raw + 0x10 * i);
            size_t len = *(size_t*)(arr_raw + 0x10 * i + 0x08);

            g_va_regions.insert(make_tuple(addr, len, va_region_names[i]));
        }
    }
    FC;

}

void dump_va_regions()
{
    try
    {
        if (g_va_regions.empty())
            init_va_regions();

        stringstream ss;

        for (auto region : g_va_regions)
        {
            ss << setw(40) << get<2>(region) << " [ "
                << showbase << hex
                << setw(18) << get<0>(region) << " - "
                << setw(18) << get<0>(region) + get<1>(region) << " , len: "
                << setw(18) << get<1>(region) << " = "
                << EXT_F_Size2Str(get<1>(region)) << "]\n";
        }

        EXT_F_OUT(ss.str().c_str());
    }
    FC;
}


string va_region_name(size_t addr)
{
    string region_name("");

    for (auto region : g_va_regions)
    {
        size_t start = get<0>(region);
        size_t len = get<1>(region);
        string name = get<2>(region);

        if (addr >= start && addr < (start + len))
        {
            region_name.swap(name);
        }
    }

    return region_name;
}

bool in_paged_pool(size_t addr)
{
    return va_region_name(addr) == "AssignedRegionPagedPool";
    //static size_t page_pool_start = 0;
 //   static size_t page_pool_len = 0;

    //try
    //{
    //	/*if (page_pool_start == 0)
    //	{
    //		size_t mi_state_addr = getSymbolAddr("nt!MiState");
    //		ExtRemoteTyped mi_state("(nt!_MI_SYSTEM_INFORMATION*)@$extin", mi_state_addr);

    //		auto va_arr = mi_state_addr;
    //		va_arr += mi_state.GetFieldOffset("Vs.SystemVaRegions");

    //		size_t bytes_read = 0;
    //		m_Data->ReadVirtual(va_arr + 0x10, &page_pool_start, 0x8, (PULONG)&bytes_read);

    //		
    //	}

    //	return (addr ^ page_pool_start) < 0x1000000000;*/

 //       if (page_pool_start == 0 || page_pool_len == 0)
 //       {
 //           for (auto region : m_va_regions)
 //           {
 //               if (get<2>(region) == "AssignedRegionPagedPool")
 //               {
 //                   page_pool_start = get<0>(region);
 //                   page_pool_len = get<1>(region);
 //               }
 //           }
 //       }

 //       if (addr >= page_pool_start && addr < (page_pool_start + page_pool_len))
 //           return true;
    //	
    //}FILTER_CATCH;

    //return false;
}

bool in_non_paged_pool(size_t addr)
{
    return va_region_name(addr) == "AssignedRegionNonPagedPool";
    //   static size_t non_page_pool_start = 0;
    //   static size_t non_page_pool_len = 0;

       //try
       //{
       //	/*if (non_page_pool_start == 0)
       //	{
       //		size_t mi_state_addr = getSymbolAddr("nt!MiState");
       //		ExtRemoteTyped mi_state("(nt!_MI_SYSTEM_INFORMATION*)@$extin", mi_state_addr);

       //		auto va_arr = mi_state_addr;
       //		va_arr += mi_state.GetFieldOffset("Vs.SystemVaRegions");

       //		size_t bytes_read = 0;
       //		m_Data->ReadVirtual(va_arr, &non_page_pool_start, 0x8, (PULONG)&bytes_read);

       //				
       //	}

       //	return (addr ^ non_page_pool_start) < 0x1000000000;*/

    //       if (non_page_pool_start == 0 || non_page_pool_len == 0)
    //       {
    //           for (auto region : m_va_regions)
    //           {
    //               if (get<2>(region) == "AssignedRegionNonPagedPool")
    //               {
    //                   non_page_pool_start = get<0>(region);
    //                   non_page_pool_len = get<1>(region);
    //               }
    //           }
    //       }

    //       if (addr >= non_page_pool_start && addr < (non_page_pool_start + non_page_pool_len))
    //           return true;

       //}FILTER_CATCH;

       //return false;
}

bool is_alpha(uint8_t ch)
{
    if (ch >= '0' && ch <= '9')
        return true;

    if (ch >= 'A' && ch <= 'Z')
        return true;

    if (ch >= 'a' && ch <= 'z')
        return true;

    return false;
}

bool in_small_pool_page(size_t addr)
{
    try
    {
        size_t page_start_addr = addr & 0xFFFFFFFFFFFFF000;

        uint8_t fields[8] = { 0, };

        size_t bytes_read = 0;
        EXT_D_IDebugDataSpaces->ReadVirtual(page_start_addr, fields, 8, (PULONG)&bytes_read);

        bool b_valid_pool_page = (fields[2] != 0 && is_alpha(fields[4]) && fields[0] == 0);

        return b_valid_pool_page;
    }
    FC;

    return false;
}

tuple<bool, size_t, string, string> as_kcode(size_t addr)
{
    if ((addr & 0xFFFF000000000000) == 0xFFFF000000000000)
    {
        try
        {
            ExtRemoteTypedList lm_list = ExtNtOsInformation::GetKernelLoadedModuleList();

            {
                for (lm_list.StartHead(); lm_list.HasNode(); lm_list.Next())
                {
                    ExtRemoteTyped lm = lm_list.GetTypedNode();
                    size_t name_addr = lm_list.GetNodeOffset() + lm.GetFieldOffset("FullDllName");
                    size_t dll_base = lm.Field("DllBase").GetUlong64();
                    size_t dll_len = lm.Field("SizeOfImage").GetUlong();

                    wstring module_name = EXT_F_READ_USTR(name_addr);
                    string str_module_name(module_name.begin(), module_name.end());
                    
                    string symbol = get<0>(EXT_F_Addr2Sym(addr));

                    size_t symbol_start = EXT_F_Sym2Addr(symbol.c_str());

                    stringstream ss;
                    ss << symbol;

                    if (addr != symbol_start)
                        ss << "+" << hex << showbase << (addr - symbol_start);

                    if (addr >= dll_base && addr < dll_base + dll_len)
                        return make_tuple(true, dll_base, str_module_name, ss.str());
                }
            }
        }
        FC;
    }

    return make_tuple(false, 0, "", "");
}

tuple<bool, size_t, string, string> as_ucode(size_t addr)
{
    if ((addr & 0xFFFF000000000000) == 0x0000000000000000)
    {
        try
        {
            ExtRemoteTypedList lm_list = ExtNtOsInformation::GetUserLoadedModuleList();

            {
                for (lm_list.StartHead(); lm_list.HasNode(); lm_list.Next())
                {
                    ExtRemoteTyped lm = lm_list.GetTypedNode();
                    size_t name_addr = lm_list.GetNodeOffset() + lm.GetFieldOffset("FullDllName");
                    size_t dll_base = lm.Field("DllBase").GetUlong64();
                    size_t dll_len = lm.Field("SizeOfImage").GetUlong();

                    wstring module_name = EXT_F_READ_USTR(name_addr);
                    string str_module_name(module_name.begin(), module_name.end());

                    string symbol = get<0>(EXT_F_Addr2Sym(addr));

                    size_t symbol_start = EXT_F_Sym2Addr(symbol.c_str());

                    stringstream ss;
                    ss << symbol;

                    if (addr != symbol_start)
                        ss << "+" << hex << showbase << (addr - symbol_start);

                    if (addr >= dll_base && addr < dll_base + dll_len)
                        return make_tuple(true, dll_base, str_module_name, ss.str());
                }
            }
        }
        FC;
    }

    return make_tuple(false, 0, "", "");
}

bool is_valid_pool_tag(uint32_t tag)
{
    uint8_t* ch = (uint8_t*)&tag;

    size_t alphas = 0;

    for (size_t i = 0; i < 4; i++)
    {
        if (is_alpha(*(ch + i)))
        {
            alphas++;
            continue;
        }

        if (*(ch + i) == 0)
            continue;

        if (*(ch + i) == '_')
            continue;

        return false;
    }

    return alphas >= 2;
}


tuple<bool, bool, bool, size_t, size_t, string> as_small_pool(size_t addr)
{
    try
    {
        size_t page_start_addr = addr & 0xFFFFFFFFFFFFF000;

        uint8_t* cur_page = new uint8_t[0x1000];

        size_t bytes_read = 0;

        EXT_D_IDebugDataSpaces->ReadVirtual(page_start_addr, cur_page, 0x1000, (PULONG)&bytes_read);

        uint8_t* next_record_ptr = cur_page;

        size_t next_record_addr = page_start_addr;

        uint8_t* cur_record = addr - page_start_addr + cur_page;

        while (next_record_ptr >= cur_page && next_record_ptr < cur_page + 0x1000)
        {
            CPoolHeader pool_header(*(size_t*)next_record_ptr);

            bool b_Valid = ((pool_header.pool_type & 2) != 0) &&
                is_valid_pool_tag(*(uint32_t*)&pool_header.tag);


            if (!b_Valid)
            {
                next_record_ptr += 0x10;
                next_record_addr += 0x10;

                continue;
            }

            string str_tag((char*)pool_header.tag, 4);

            /*stringstream ss;
            ss << endl << "Pool Header at "
                << showbase << hex << setw(18) << next_record_addr
                << " tag: " << str_tag << endl;

            Out(ss.str().c_str());

            ss.str("");

            ss << "dt nt!_POOL_HEADER " << showbase << hex << setw(18) << next_record_addr;
            x(ss.str().c_str());*/

            if (cur_record >= next_record_ptr && cur_record < next_record_ptr + pool_header.block_size * 0x10)
            {
                bool b_Paged_or_nonpaged = ((pool_header.pool_type & 1) == 1);

                bool b_Allocated_or_free = !((pool_header.block_size == 0) || (str_tag == "Free"));

                delete[] cur_page;
                return make_tuple(true, b_Paged_or_nonpaged, b_Allocated_or_free,
                    next_record_addr + 0x10,
                    pool_header.block_size * 0x10 - 0x10,
                    str_tag);
            }

            next_record_ptr += (pool_header.block_size == 0 ? 1 : pool_header.block_size + 1) * 0x10;
            next_record_addr += (pool_header.block_size == 0 ? 1 : pool_header.block_size + 1) * 0x10;
        };

        delete[] cur_page;
    }
    FC;

    return make_tuple(false, false, false, 0, 0, "");
}

tuple<bool, bool, bool, size_t, size_t, string> as_large_pool(size_t addr)
{
    uint8_t* page_x3_buffer = new uint8_t[0x3000];

    try
    {
        size_t big_pool_addr = EXT_F_READ<size_t>(EXT_F_Sym2Addr("nt!PoolBigPageTable"));
        size_t big_pool_size = EXT_F_READ<size_t>(EXT_F_Sym2Addr("nt!PoolBigPageTableSize"));

        stringstream ss;

        size_t item_count = 0;
        size_t page_x3_index = 0;

        for (size_t item_addr = big_pool_addr; item_count < big_pool_size; item_addr += 0x18)
        {
            if ((item_count % 0x200) == 0)
            {
                size_t bytes_read = 0;
                EXT_D_IDebugDataSpaces->ReadVirtual(big_pool_addr + page_x3_index * 0x3000, page_x3_buffer, 0x3000, (PULONG)&bytes_read);

                page_x3_index++;
            }

            item_count++;

            CBigPoolHeader pool_entry(page_x3_buffer + 0x18 * (item_count % 0x200));

            size_t pool_va = pool_entry.va & 0xFFFFFFFFFFFFF000;

            if (pool_va != pool_entry.va)
                continue;

            if (like_kaddr(pool_entry.va))
            {
                if ((addr >= pool_va) && (addr < (pool_va + pool_entry.size)))
                {
                    bool b_free = ((pool_entry.va & 0x1) == 0x1);

                    bool b_Paged_or_nonpaged = ((pool_entry.pool_type & 1) == 1);
                    string pool_tag = pool_entry.tag;
                    bool b_Allocated_or_free = !(b_free || (pool_entry.size == 0) || (pool_tag == "Free"));

                    delete[] page_x3_buffer;
                    return make_tuple(false, b_Paged_or_nonpaged, b_Allocated_or_free, pool_va, pool_entry.size, pool_tag);
                }
            }
        }
    }
    FC;

    delete[] page_x3_buffer;

    return make_tuple(false, false, false, 0, 0, "");
}

string dump_plain_qword(size_t curr_qword)
{
    stringstream ss;
    string raw((char*)&curr_qword, 8);
    for (char& ch : raw)
    {
        if (EXT_F_IS_IN_RANGE<char>(ch, 'A', 'Z') || EXT_F_IS_IN_RANGE<char>(ch, 'a', 'z') || EXT_F_IS_IN_RANGE<char>(ch, '0', '9'))
        {
        }
        else
        {
            ch = '.';
        }
    }

    ss << hex << noshowbase << setfill('0')
        << "[ " << setw(8) << (curr_qword & 0xFFFFFFFF) << " " << setw(8) << ((curr_qword >> 0x20) & 0xFFFFFFFF) << " ]\t"
        << "[ " << setw(2) << ((curr_qword >> 0) & 0xFF) << " " << setw(2) << ((curr_qword >> 8) & 0xFF)
        << " " << setw(2) << ((curr_qword >> 16) & 0xFF) << " " << setw(2) << ((curr_qword >> 24) & 0xFF)
        << " " << setw(2) << ((curr_qword >> 32) & 0xFF) << " " << setw(2) << ((curr_qword >> 40) & 0xFF)
        << " " << setw(2) << ((curr_qword >> 48) & 0xFF) << " " << setw(2) << ((curr_qword >> 56) & 0xFF)
        << " |" << raw << "| "
        << (curr_qword != 0 ? "*" : " ")
        << " ]" << setfill(' ');

    return ss.str();
}

typedef enum _MI_VAD_TYPE {
    VadNone,
    VadDevicePhysicalMemory,
    VadImageMap,
    VadAwe,
    VadWriteWatch,
    VadLargePages,
    VadRotatePhysical,
    VadLargePageSection
} MI_PFN_CACHE_ATTRIBUTE, * PMI_PFN_CACHE_ATTRIBUTE;

map<uint32_t, string> vadTypeMap{ {
    { VadNone , "VadNone"},
    { VadDevicePhysicalMemory , "VadDevicePhysicalMemory"},
    { VadImageMap , "VadImageMap"},
    { VadAwe , "VadAwe"},
    { VadWriteWatch , "VadWriteWatch"},
    { VadLargePages , "VadLargePages"},
    { VadRotatePhysical , "VadRotatePhysical"},
    { VadLargePageSection , "VadLargePageSection"}
} };

void dump_vad(size_t root_addr)
{
    try
    {
        if (root_addr == 0)
            return;

        ExtRemoteTyped mmvad("(nt!_MMVAD*)@$extin", root_addr);

        size_t left = mmvad.Field("Core.VadNode.Left").GetUlong64();
        visit_vad(left);

        size_t right = mmvad.Field("Core.VadNode.Right").GetUlong64();
        visit_vad(right);
    }
    FC;
}

void visit_vad(size_t vad_node_addr)
{
    try
    {
        if (vad_node_addr == 0)
            return;

        ExtRemoteTyped mmvad("(nt!_MMVAD*)@$extin", vad_node_addr);

        size_t left = mmvad.Field("Core.VadNode.Left").GetUlong64();

        visit_vad(left);

        size_t start_vpn = mmvad.Field("Core.StartingVpn").GetUlong();
        size_t end_vpn = mmvad.Field("Core.EndingVpn").GetUlong();

        size_t start_vpn_high = mmvad.Field("Core.StartingVpnHigh").GetUchar();
        if (start_vpn_high != 0)
        {
            start_vpn |= (start_vpn_high << 32);
        }

        size_t end_vpn_high = mmvad.Field("Core.EndingVpnHigh").GetUchar();
        if (end_vpn_high)
        {
            end_vpn |= (end_vpn_high << 32);
        }

        size_t subsection = mmvad.Field("Subsection").GetUlong64();
        size_t first_proto_pte = mmvad.Field("FirstPrototypePte").GetUlong64();
        size_t last_pte = mmvad.Field("LastContiguousPte").GetUlong64();

        size_t vads_process = mmvad.Field("VadsProcess").GetUlong64();
        size_t file_obj = mmvad.Field("FileObject").GetUlong64();

        uint32_t flags = mmvad.Field("Core.u.LongFlags").GetUlong();

        bool b_private = ((flags & (1 << 20)) != 0);
        bool b_graphics = ((flags & (1 << 24)) != 0);

        size_t vad_type = ((flags >> 4) & 0x7);
        size_t protection = ((flags >> 7) & 0x1F);

        bool b_mapped = (vad_type == VadImageMap);

        stringstream ss;

        ss << hex << noshowbase
            << " <link cmd=\"dt nt!_MMVAD 0x" << setfill('0') << setw(16) << vad_node_addr
            << "\">0x"
            << setfill('0') << setw(16) << vad_node_addr
            << "</link>"
            << setfill(' ') << showbase
            << "\t"
            << setw(16) << start_vpn
            << " - "
            << setw(16) << end_vpn
            << " Flags : " << setw(10) << flags
            << setw(10) << (b_private ? " Private " : " Shared ")
            //<< setw(10) << (b_graphics ? " Graphics " : " ")
            << " " << setw(8) << vad_type;

        auto it = vadTypeMap.find((uint32_t)vad_type);
        if (!b_private && it != vadTypeMap.end() && vad_type != VadNone)
            ss << " " << setw(14) << it->second;
        else
            ss << " " << setw(14) << "";

        ss << " / " << setw(8) << protection;

        if (b_mapped && (file_obj != 0))
        {
            auto wfile_name = dump_file_name(file_obj);
            ss << string(wfile_name.begin(), wfile_name.end());
        }
        //<< noshowbase
        //<< " <link cmd=\"dt nt!_SUBSECTION 0x" << setfill('0') << setw(16) << subsection
        //<< "\">0x"
        //<< setfill('0') << setw(16) << subsection
        //<< "</link>"
        //<< " <link cmd=\"!dk page 0x" << setfill('0') << setw(16) << first_proto_pte
        //<< "\">0x"
        //<< setfill('0') << setw(16) << first_proto_pte
        //<< "</link>"
        //<< " <link cmd=\"dt nt!_FILE_OBJECT 0x" << setfill('0') << setw(16) << file_obj
        //<< "\">0x"
        //<< setfill('0') << setw(16) << file_obj
        //<< "</link> "
        ss << endl;

        EXT_F_DML(ss.str().c_str());

        size_t right = mmvad.Field("Core.VadNode.Right").GetUlong64();
        visit_vad(right);
    }
    FC;
}

void do_memcpy(size_t src_addr, size_t dst_addr, size_t count)
{
    try
    {
        size_t qwords = count / 0x08;
        size_t bytes = count % 0x08;

        for (size_t i = 0; i < qwords; i++)
        {
            size_t temp_qword = EXT_F_READ<size_t>(src_addr + i * 0x08);
            EXT_F_WRITE<size_t>(dst_addr + i * 0x08, temp_qword);
        }

        for (size_t i = 0; i < bytes; i++)
        {
            uint8_t byte = EXT_F_READ<uint8_t>(src_addr + qwords * 0x08 + i);
            EXT_F_WRITE<uint8_t>(dst_addr + qwords * 0x08 + i, byte);
        }
    }
    FC;
}
