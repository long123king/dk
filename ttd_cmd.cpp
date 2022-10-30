#include "ttd_cmd.h"
#include "model.h"
#include "CmdExt.h"
#include "CmdList.h"

#include <iomanip>

// {b1d2e6ab_9052_4b72_999e_a88ba868f899}
const GUID IID_ICursor = { 0xb1d2e6ab, 0x9052, 0x4b72, { 0x99, 0x9e, 0xa8, 0x8b, 0xa8, 0x68, 0xf8, 0x99 } };

// {4d3420a5_37ef_4114_ae91_63d0378c84a9}
const GUID IID_IReplayEngine = { 0x4d3420a5, 0x37ef, 0x4114, { 0xae, 0x91, 0x63, 0xd0, 0x37, 0x8c, 0x84, 0xa9 } };

                            
DEFINE_CMD(ldttd)
{	    
    if (!DK_MODEL_ACCESS->isTTD())
    {
        EXT_F_OUT("Usage: !dk ldttd <min_seq> <min_step> <max_seq> <max_step>\nTTD Mode Only\n");
        return;
    }


    CTTDInsight::Instance()->load_ttd();
}

DEFINE_CMD(dump_ttd_events)
{
    if (!DK_MODEL_ACCESS->isTTD() || args.size() < 2)
    {
        EXT_F_OUT("Usage: !dk dump_ttd_events <output_filename>\nTTD Mode Only\n");
        return;
    }
    string out_filename = args[1];

    CTTDInsight::Instance()->dump_ttd_model_events(out_filename);
}

void CTTDInsight::ttd_call_ret_callback(
    uintptr_t context,
    TTD::GuestAddress guestInstructionAddress,
    TTD::GuestAddress guestFallThroughInstructionAddress,
    TTD::TTD_Replay_IThreadView* threadView)
{
    static size_t callret_count = 0;
    callret_count++;

    ttd_event event;

    event.tid = (uint64_t)threadView->IThreadView->GetThreadInfo(threadView)->threadid;
    event.position.sequence_id = (uint64_t)threadView->IThreadView->GetPosition(threadView)->Major;
    event.position.step_id = (uint64_t)threadView->IThreadView->GetPosition(threadView)->Minor;
    //event.prev_position.sequence_id = (uint64_t)threadView->IThreadView->GetPreviousPosition(threadView)->Major;
    //event.prev_position.step_id = (uint64_t)threadView->IThreadView->GetPreviousPosition(threadView)->Minor;
    event.pc = (uint64_t)threadView->IThreadView->GetProgramCounter(threadView);
    event.stack_pointer = (uint64_t)threadView->IThreadView->GetStackPointer(threadView);
    event.frame_pointer = (uint64_t)threadView->IThreadView->GetFramePointer(threadView);

    event.event_type = ttd_event::TTD_CALLRET_EVENT;
    event.u_data.callret_event.insn_addr = (uint64_t)guestInstructionAddress;
    event.u_data.callret_event.next_addr = (uint64_t)guestFallThroughInstructionAddress;

    m_ttd_events.emplace_back(event);    
}

void __fastcall MyCallReturnCallback(unsigned __int64 callback_value,
    TTD::GuestAddress addr_func,
    TTD::GuestAddress addr_ret,
    struct TTD::TTD_Replay_IThreadView* thread_view)
{
    ((CTTDInsight*)(callback_value))->ttd_call_ret_callback(callback_value, addr_func, addr_ret, thread_view);
}

PVOID QueryNativeInterface(IID* iid)
{
    WDBGEXTS_QUERY_INTERFACE interface_info = {};
    interface_info.Iid = iid;

    auto hr = Ioctl(IG_QUERY_TARGET_INTERFACE, &interface_info, sizeof(interface_info));
    if (hr == 0)
    {
        return nullptr;
    }

    return interface_info.Iface;
}

void CTTDInsight::load_ttd()
{
    m_ttd_events.clear();
    TTD::TTD_Replay_ReplayEngine* ifc_IReplayEngine =
        reinterpret_cast<TTD::TTD_Replay_ReplayEngine*>(QueryNativeInterface((IID*)&IID_IReplayEngine));
    TTD::TTD_Replay_ICursor* ifc_ICursor =
        reinterpret_cast<TTD::TTD_Replay_ICursor*>(QueryNativeInterface((IID*)&IID_ICursor));

    if (ifc_IReplayEngine != nullptr && ifc_ICursor != nullptr)
    {
        TTD::Position* pos_begin = ifc_IReplayEngine->IReplayEngine->GetFirstPosition(ifc_IReplayEngine);
        TTD::Position* pos_end = ifc_IReplayEngine->IReplayEngine->GetLastPosition(ifc_IReplayEngine);

        ifc_ICursor->ICursor->SetCallReturnCallback(ifc_ICursor, (TTD::PROC_CallCallback)MyCallReturnCallback, (uintptr_t)this);

        ifc_ICursor->ICursor->SetReplayFlags(ifc_ICursor, 7);

        ifc_ICursor->ICursor->SetPosition(ifc_ICursor, pos_begin);

        TTD::TTD_Replay_ICursorView_ReplayResult replayrez;
        ifc_ICursor->ICursor->ReplayForward(ifc_ICursor, &replayrez, pos_end, -1);
    }
}

string make_rgb(uint64_t r, uint64_t g, uint64_t b)
{
    stringstream ss;

    ss << "#" << setfill('0') << noshowbase
        << setw(2) << r % 256
        << setw(2) << g % 256
        << setw(2) << b % 256;

    return ss.str();
}

string make_heat_color(uint64_t heat)
{      
    vector<string> pallete = {
        "white",
        "#74D2EE",
        "#00A2B9",
        "green",
        "#FFCC40",
        "#FC6C00",
    };
            
    uint64_t index = log2(heat + 1);
    if (index < pallete.size())
        return pallete[index];
    else
        return "#B01111";
}

string make_heat_style_class(uint64_t heat)
{
    vector<string> pallete = {
        "heat_0",
        "heat_1",
        "heat_2",
        "heat_3",
        "heat_4",
        "heat_5",
    };

    uint64_t index = log2(heat + 1);
    if (index < pallete.size())
        return pallete[index];
    else
        return "heat_6";
}

void CTTDInsight::dump_ttd_model_events(string out_filename)
{
    CTTDModelCallstackBuilder callstack_builder;

    callstack_builder.build_ttd_callstack_model();

    for (auto root : callstack_builder.m_ttd_call_forest)
    {
        for (auto& branch : root.second)
        {
            callstack_builder.calc_seqs_count(branch);
        }
    }

    string svg_filename = out_filename + "_forest.svg";

    auto rect_g = make_shared<CSvgGroup>();
    rect_g->addStyle("fill:white; stroke:black;");

    auto text_g = make_shared<CSvgGroup>();
    text_g->addStyle("font-family: monospace; font-size: 16;");

    auto tag_g = make_shared<CSvgGroup>();

    uint64_t x = 100;
    uint64_t y = 100;
    uint64_t max_x = x;
    const uint64_t min_width = 1000;
    const uint64_t height = 40;
    const uint64_t text_disp = 20;
    for (auto& root : callstack_builder.m_ttd_call_forest)
    {
        for (auto& branch : root.second)
        {
            stringstream branch_ss;
            branch_ss << out_filename << "_" << hex << root.first << "_" << branch->func_name << ".svg";

            callstack_builder.build_callstack_svg(branch_ss.str(), branch);

            string style = "fill: ";
            style += make_heat_color(branch->seqs_count);
            style += ";";

            if (branch->func_name.find("ClassMemoryModel!") == 0)
            {
                auto tag = make_shared<CSvgRect>(CSvgPoint(x - 40, y), 40 + min_width + log2(branch->seqs_count) * 100, height, "fill: red; stroke: black; stroke-width: 1; fill-opacity: 0.3;");

                tag_g->appendElement(tag);

                //style += "stroke: red; stroke-width: 10;";
            }
            //else
            {
                style += "stroke: black; stroke-width: 1;";
            }

            auto rect = make_shared<CSvgRect>(CSvgPoint(x, y), min_width + log2(branch->seqs_count) * 100, height, style);

            auto rect_link = make_shared<CSvgLink>(branch_ss.str());

            rect_link->appendElement(rect);

            if (max_x < branch->seqs_count * 100 + min_width)
                max_x = branch->seqs_count * 100 + min_width;

            rect_g->appendElement(rect_link);

            stringstream ss;

            ss << hex << " " << root.first
                << " ( " << hex << setfill(' ')
                << right << setw(4) << branch->start_pos.sequence_id << ":"
                << left << setw(4) << branch->start_pos.step_id << " - "
                << right << setw(4) << branch->end_pos.sequence_id << ":"
                << left << setw(4) << branch->end_pos.step_id << " ) ";

            for (auto ch : branch->func_name)
            {
                if (ch == '<')
                    ss << "&lt;";
                else if (ch == '>')
                    ss << "&gt;";
                else if (ch == '&')
                    ss << "&amp;";
                else
                    ss << ch;
            }


            auto text = make_shared<CSvgText>(CSvgPoint(x + 50, y + text_disp), ss.str());

            text_g->appendElement(text);

            y += height;
        }
    }

    y += height;

    auto svg_doc = make_shared<CSvgDoc>(max_x + 2 * height, y, CSvgPoint(0, 0), max_x, y);

    svg_doc->appendElement(rect_g);
    svg_doc->appendElement(text_g);
    svg_doc->appendElement(tag_g);

    svg_doc->Save(svg_filename);

}

string CTTDInsight::dump_ttd_callnode(DK_TTD_CALLNODE callnode, size_t level)
{
    stringstream ss;

    if (callnode)
    {
        ss << string(level * 8, ' ') << callnode->func_name;
        ;

        uint64_t max_seq = 0;
        uint64_t min_seq = 0;

        ss << "\t[ ";
        if (callnode->call_event_index >= 0 && callnode->call_event_index < m_ttd_events.size())
        {
            auto event = m_ttd_events[callnode->call_event_index];

            min_seq = event.position.sequence_id;

            ss << hex << event.position.sequence_id << ":" << event.position.step_id << " ";
        }

        ss << " - ";

        if (callnode->return_event_index >= 0 && callnode->return_event_index < m_ttd_events.size())
        {
            auto event = m_ttd_events[callnode->return_event_index];

            max_seq = event.position.sequence_id;

            ss << "" << hex << event.position.sequence_id << ":" << event.position.step_id;
        }

        if (max_seq <= min_seq)
            callnode->seqs_count = 1;
        else
            callnode->seqs_count = (max_seq - min_seq);


        ss << " ] ( " << callnode->seqs_count << " ) " << endl;

        for (auto child : callnode->children)
        {
            ss << dump_ttd_callnode(child, level + 1);
        }

        if (!callnode->callstack.empty())
        {
            ss << callnode->callstack;
        }
    }

    return ss.str();
}

void CTTDModelCallstackBuilder::calc_seqs_count(DK_TTD_CALLNODE node)
{
    if (node)
    {
        uint64_t max_seq = 0;
        uint64_t min_seq = 0;

        if (node->call_event_index >= 0 && node->call_event_index < CTTDInsight::Instance()->m_ttd_events.size())
        {
            min_seq = CTTDInsight::Instance()->m_ttd_events[node->call_event_index].position.sequence_id;
        }

        if (node->return_event_index >= 0 && node->return_event_index < CTTDInsight::Instance()->m_ttd_events.size())
        {
            max_seq = CTTDInsight::Instance()->m_ttd_events[node->return_event_index].position.sequence_id;
        }

        if (max_seq <= min_seq)
            node->seqs_count = 1;
        else
            node->seqs_count = (max_seq - min_seq);

        for (auto child : node->children)
        {
            calc_seqs_count(child);
        }
    }
}

DK_TTD_CALLNODE CTTDModelCallstackBuilder::is_func_in_chain(tuple<string, uint64_t> func, DK_TTD_CALLNODE chain)
{
    auto frame = chain;
    while (frame != nullptr)
    {
        if (frame->caller_name == get<0>(func) && frame->caller_offset == get<1>(func))
        {
            return frame->parent;
        }

        frame = frame->parent;
    };

    return nullptr;
}

tuple<string, uint64_t> CTTDModelCallstackBuilder::addr2sym(uint64_t addr)
{
    auto sym = EXT_F_Addr2Sym(addr);
    if (get<0>(sym).empty())
    {
        stringstream ss;
        ss << "0x" << setfill('0') << setw(16) << hex << addr;
        sym = make_tuple(ss.str(), 0);
    }
    return sym;
}

DK_TTD_CALLNODE CTTDModelCallstackBuilder::fix_missing_chain(DK_TTD_CALLNODE root_chain, ttd_position pos, tuple<string, uint64_t> orphan_event_node)
{
    dump_root_chain(root_chain);

    DK_TTD_CALLNODE joint_node = nullptr;
    DK_TTD_CALLNODE orphan_node = nullptr;
    vector<DK_TTD_CALLNODE> missing_chain;

    DK_SEEK_TO(pos.sequence_id, pos.step_id);
    auto callstacks = DK_GET_CURSTACK();

    auto cur_tid = DK_GET_CURTID();

#ifdef DEBUG
    string callstack_str = DK_DUMP_CURSTACK();
#endif

    bool b_orphan_found_in_stack = false;

    for (auto& callstack : callstacks)
    {
        auto real_frame = addr2sym(callstack[1]);
        if (b_orphan_found_in_stack)
        {
            joint_node = is_func_in_chain(real_frame, root_chain);
            if (joint_node == nullptr)
            {
                auto frame_node = make_shared<ttd_callnode>();

                frame_node->caller_name = "";
                frame_node->caller_offset = 0;

                frame_node->call_event_index = -1;
                frame_node->parent = nullptr;

                frame_node->func_name = get<0>(addr2sym(callstack[1]));

                frame_node->start_pos.sequence_id = pos.sequence_id;
                frame_node->start_pos.step_id = pos.step_id;

                if (!missing_chain.empty())
                {
                    auto chain_last = *missing_chain.rbegin();

                    chain_last->children.push_back(frame_node);
                    frame_node->parent = chain_last;
                    frame_node->caller_name = chain_last->func_name;
                }

                missing_chain.push_back(frame_node);
            }
            else
            {
                // End the search towards root.

                // build chain from node_in_chain->[reverse of missing_chain]->frame
                if (missing_chain.empty())
                {
                    joint_node->children.push_back(orphan_node);
                    orphan_node->parent = joint_node;
                    orphan_node->caller_name = joint_node->func_name;
                }
                else
                {
                    auto chain_first = *missing_chain.begin();
                    auto chain_last = *missing_chain.rbegin();

                    joint_node->children.push_back(chain_first);
                    chain_first->parent = joint_node;
                    chain_first->caller_name = joint_node->func_name;

                    chain_last->children.push_back(orphan_node);
                    orphan_node->parent = chain_last;
                    orphan_node->caller_name = chain_last->func_name;
                }

                break;
            }
        }
        else if (get<0>(real_frame) == get<0>(orphan_event_node))
        {
            // Fix the gap between root chain and callstack chain.
            // root chain may not join with callstack chain with more than one missing node.
            orphan_node = make_shared<ttd_callnode>();

            orphan_node->caller_name = "";
            orphan_node->caller_offset = 0;

            orphan_node->call_event_index = -1;
            orphan_node->parent = nullptr;

            orphan_node->func_name = get<0>(orphan_event_node);

            orphan_node->start_pos.sequence_id = pos.sequence_id;
            orphan_node->start_pos.step_id = pos.step_id;

            b_orphan_found_in_stack = true;
        }
    }

    if (joint_node == nullptr && orphan_node != nullptr)
    {
        m_ttd_call_forest[cur_tid].push_back(orphan_node);
        m_ttd_lastnodes[cur_tid] = orphan_node;
    }

    return orphan_node;
}

void CTTDModelCallstackBuilder::dump_root_chain(DK_TTD_CALLNODE root_chain)
{
    m_root_chain.clear();

#ifdef DEBUG
    stringstream ss;
    ss << "Root chain:" << endl;
#endif
    auto frame = root_chain;
    while (frame != nullptr)
    {
        m_root_chain.push_back(frame->func_name);
#ifdef DEBUG
        ss << frame->func_name << endl;
#endif
        frame = frame->parent;
    };

#ifdef DEBUG
    EXT_F_STR_OUT(ss);
#endif
}

tuple<DK_TTD_CALLNODE, DK_TTD_CALLNODE> CTTDModelCallstackBuilder::build_new_root_chain(ttd_position pos)
{
    DK_TTD_CALLNODE root = nullptr;
    DK_TTD_CALLNODE last_node = nullptr;

    DK_SEEK_TO(pos.sequence_id, pos.step_id);
    auto init_callstack = DK_GET_CURSTACK();

    vector<array<uint64_t, 5>> reverse_callstack(init_callstack.rbegin(), init_callstack.rend());

    // frame_number, insn_ptr, return_ptr, frame_ptr, stack_ptr
    for (auto& frame : reverse_callstack)
    {
        auto frame_node = make_shared<ttd_callnode>();

        if (frame[2] == 0)
        {
            frame_node->caller_name = "<out-of-scope-caller>";
            frame_node->caller_offset = 0;
        }
        else
        {
            auto sym_ret = addr2sym(frame[2]);
            frame_node->caller_name = get<0>(sym_ret);
            frame_node->caller_offset = get<1>(sym_ret);
        }

        frame_node->call_event_index = -1;
        frame_node->parent = nullptr;

        frame_node->func_name = get<0>(addr2sym(frame[1]));

        auto cur_tid = DK_GET_CURTID();

        if (root == nullptr)
        {
            root = frame_node;
        }

        if (last_node != nullptr)
        {
            last_node->children.push_back(frame_node);
            frame_node->parent = last_node;
        }

        last_node = frame_node;
    }

    return make_tuple(root, last_node);
}

void CTTDModelCallstackBuilder::build_callstack_svg(string filename, DK_TTD_CALLNODE root)
{
    m_rect_g = make_shared<CSvgGroup>();
    m_rect_g->addStyle("fill:white; stroke:black;");
    m_rect_g->setId("rect_g");

    m_text_g = make_shared<CSvgGroup>();
    m_text_g->addStyle("font-family: monospace; font-size: 16;");
    m_text_g->setId("text_g");

    auto inner_style = make_shared<CSvgInnerStyle>();

    map<string, string> fill_pallete = {
    {"rect.heat_0",     "fill: white;"},
    {"rect.heat_1",     "fill: #74D2EE;"},
    {"rect.heat_2",     "fill: #00A2B9;"},
    {"rect.heat_3",     "fill: green;"},
    {"rect.heat_4",     "fill: #FFCC40;"},
    {"rect.heat_5",     "fill: #FC6C00;"},
    {"rect.heat_6",     "fill: #B01111;"},
    {"rect",            "stroke: black; stroke-width: 1;"},
    {"rect.fold",       "stroke: silver; stroke-width: 15px; stroke-linecap: round;"},
    {"text",            "white-space: pre;"},
    };

    for (auto it : fill_pallete)
    {
        inner_style->addClassStyle(it.first, it.second);
    }

    auto inner_script = make_shared<CSvgInnerScript>();

    string script = R"(	

    function show_rect(rect)
	{
		rect.setAttribute("stroke-width", "1px");
		rect.setAttribute("fill-opacity", 100);
		rect.setAttribute("stroke-opacity", 100);
		rect.removeAttribute("state");		
	}

	function hide_rect(rect)
	{
		rect.setAttribute("fill-opacity", 0);
		rect.setAttribute("stroke-opacity", 0);
		rect.setAttribute("state", "hidden");
	}

	function update_position(rect)
	{
		var index = parseInt(rect.getAttribute("Id"));

		var rect_group = document.getElementById("rect_g");
		var text_group = document.getElementById("text_g");

		var height = 40;

		var rects = rect_group.children;
		var texts = text_group.children;
		var b_found_choice = false;

		var accum_height = 0;
		for (var i=0;i<rects.length;i++)
		{			
			if (parseInt(rects[i].getAttribute("fill-opacity")) != 0)
			{
				accum_height += height;
				rects[i].setAttribute("y", accum_height);
				texts[i].setAttribute("y", accum_height+20);
			}
		}	

		var root = document.getElementById("root");
		var root_width = root.getAttribute("width");
		var root_height = accum_height + 100;
		root.setAttribute("height", root_height);
		root.setAttribute("viewBox", "0 0 " + root_width + " " + root_height);
	}

	function show_children(rect)
	{
		var index = parseInt(rect.getAttribute("Id"));

		var rect_group = document.getElementById("rect_g");
		var text_group = document.getElementById("text_g");
		var x_filter = parseInt(rect.getAttribute("x"));

		var rects = rect_group.children;
		var b_found_choice = false;
		for (var i=0;i<rects.length;i++)
		{			
			if (i == index)
			{
				b_found_choice = true;
			}
			if (b_found_choice)
			{
				if(parseInt(rects[i].getAttribute("x")) > x_filter)
				{
					show_rect(rects[i]);
					text_group.children[i].setAttribute("stroke-opacity", 100);
					text_group.children[i].setAttribute("fill-opacity", 100);
				}
				else if(i > index)
				{
					b_found_choice = false;
				}
			}
		}	
	}

	function hide_children(rect)
	{
		var index = parseInt(rect.getAttribute("Id"));

		var rect_group = document.getElementById("rect_g");
		var text_group = document.getElementById("text_g");
		var x_filter = parseInt(rect.getAttribute("x"));

		var rects = rect_group.children;
		var b_found_choice = false;
		for (var i=0;i<rects.length;i++)
		{			
			if (i == index)
			{
				b_found_choice = true;
			}
			if (b_found_choice)
			{
				if(parseInt(rects[i].getAttribute("x")) > x_filter)
				{
					hide_rect(rects[i])
					text_group.children[i].setAttribute("stroke-opacity", 0);
					text_group.children[i].setAttribute("fill-opacity", 0);
				}
				else if(i > index)
				{
					b_found_choice = false;
				}
			}
		}	
	}

	function rect_click(evt) 
	{
		var choice = evt.target;

		var style_class = choice.getAttribute("class");

		if (style_class.endsWith(" fold"))
		{
			show_children(choice);

			choice.setAttribute("class", style_class.slice(0, -5));
		}
		else
		{
			hide_children(choice);

			choice.setAttribute("class", style_class + " fold");
		}

		update_position(choice);
	}

	function init(evt)
	{
		var rect_group = document.getElementById("rect_g");

		var rects = rect_group.children;
		for (var i=0;i<rects.length;i++)
		{
			rects[i].setAttribute("Id", i);
			rects[i].addEventListener("click", rect_click, false);
		}		
	}

    )";

    inner_script->addScript(script);

    append_callstack_svg_node(root, CSvgPoint(100, 100));

    m_canvas_height = (m_canvas_lines + 2) * 40;

    auto svg_doc = make_shared<CSvgDoc>(m_canvas_width, m_canvas_height, CSvgPoint(0, 0), m_canvas_width, m_canvas_height);

    svg_doc->appendElement(inner_style);
    svg_doc->appendElement(inner_script);
    svg_doc->appendElement(m_rect_g);
    svg_doc->appendElement(m_text_g);

    svg_doc->Save(filename);

    m_rect_g = nullptr;
    m_text_g = nullptr;

    m_canvas_lines = 1;
}

void CTTDModelCallstackBuilder::append_callstack_svg_node(DK_TTD_CALLNODE node, CSvgPoint pivot)
{
    const uint64_t min_width = 1000;
    const uint64_t height = 40;
    const uint64_t text_disp = 20;

    pivot.m_y = m_canvas_lines * height;

    string style = "fill: ";
    style += make_heat_color(node->seqs_count);
    style += ";";
    style += "stroke: black; stroke-width: 1;";


    stringstream ss;

    ss << " ( " << hex << setfill(' ')
        << right << setw(4) << node->start_pos.sequence_id << ":"
        << left << setw(4) << node->start_pos.step_id << " - "
        << right << setw(4) << node->end_pos.sequence_id << ":"
        << left << setw(4) << node->end_pos.step_id << " ) ";

    for (auto ch : node->func_name)
    {
        if (ch == '<')
            ss << "&lt;";
        else if (ch == '>')
            ss << "&gt;";
        else if (ch == '&')
            ss << "&amp;";
        else
            ss << ch;
    }

    auto text = make_shared<CSvgText>(CSvgPoint(pivot.m_x + 50, pivot.m_y + text_disp), ss.str());

    m_text_g->appendElement(text);

    auto rect = make_shared<CSvgRect>(pivot, max(min_width + 100, ss.str().size() * 8), height, "", make_heat_style_class(node->seqs_count));

    if (m_canvas_width < max(min_width + 100, ss.str().size() * 8) + pivot.m_x)
        m_canvas_width = max(min_width + 100, ss.str().size() * 8) + pivot.m_x;

    m_canvas_height += height;

    m_rect_g->appendElement(rect);

    for (auto& child : node->children)
    {
        m_canvas_lines++;

        append_callstack_svg_node(child, CSvgPoint(pivot.m_x + 100, 100));
    }
}

void CTTDModelCallstackBuilder::build_ttd_callstack_model()
{
    DK_TTD_CALLNODE root = nullptr;
    uint64_t root_tid = 0;
    DK_TTD_CALLNODE last_node = nullptr;
    size_t index = 0;

    ttd_position init_pos = { 0, 0 };
    auto init_root_chain = build_new_root_chain(init_pos);

    root = get<0>(init_root_chain);
    last_node = get<1>(init_root_chain);

    root_tid = DK_GET_CURTID();
    m_ttd_call_forest[root_tid].push_back(root);
    m_ttd_lastnodes[root_tid] = last_node;

    auto total_events = CTTDInsight::Instance()->m_ttd_events.size();

    for (auto& event : CTTDInsight::Instance()->m_ttd_events)
    {
        if (index % 1000 == 0)
        {
            EXT_F_OUT("Handle %d / %d events\n", index, total_events);
        }

        auto cur_tid = event.tid;

        if (cur_tid != root_tid)
        {
            EXT_F_OUT("[New Root Chain] old_tid: %x, new_tid: %x, position (%x : %x)\n", root_tid, cur_tid, event.position.sequence_id, event.position.step_id);

            root_tid = cur_tid;

            auto new_root_chain = build_new_root_chain(event.position);

            if (m_ttd_call_forest[cur_tid].empty() || (*(m_ttd_call_forest[cur_tid].rbegin()) != get<0>(new_root_chain)))
            {
                root = get<0>(new_root_chain);

                m_ttd_call_forest[root_tid].push_back(root);
                m_ttd_lastnodes[root_tid] = get<1>(new_root_chain);
            }
        }

        if (event.event_type == ttd_event::TTD_CALLRET_EVENT)
        {
            auto sym_insn = addr2sym(event.u_data.callret_event.insn_addr);

            if (event.u_data.callret_event.next_addr == 0)
            {
                // return                           

                auto ret_node = m_ttd_lastnodes[cur_tid];

                auto sym_pc = addr2sym(event.pc);

                if (ret_node != nullptr && ret_node->caller_name == get<0>(sym_insn))
                {
                    auto return_to_node = ret_node->parent;
                    if (return_to_node != nullptr)
                    {
                        if (ret_node->caller_name == return_to_node->func_name)
                        {
                            // [likely]
                            ret_node->return_event_index = index;
                            ret_node->end_pos.sequence_id = event.position.sequence_id;
                            ret_node->end_pos.step_id = event.position.step_id;

                            m_ttd_lastnodes[cur_tid] = return_to_node;

                        }

                    };

                    if (return_to_node == nullptr)
                    {
                        // [unlikely]
                        // No pivot found in callstack, figure out if any missing chain.

                        return_to_node = fix_missing_chain(m_ttd_lastnodes[cur_tid], event.position, sym_insn);

                        if (return_to_node != nullptr)
                        {
                            return_to_node->call_event_index = index;
                            return_to_node->return_event_index = index;
                            m_ttd_lastnodes[cur_tid] = return_to_node;
                        }
                    }

#ifdef DEBUG
                    stringstream log_ss;
                    log_ss << " [ " << hex << setw(4) << index << " ] - "
                        << get<0>(sym_insn) << endl;
                    EXT_F_STR_OUT(log_ss);
#endif
                }
            }
            else
            {
                // call
                auto sym_next = addr2sym(event.u_data.callret_event.next_addr);

                auto cur_tid = event.tid;
                auto caller_node = m_ttd_lastnodes[cur_tid];
                while (caller_node != nullptr)
                {
                    if (caller_node->func_name == get<0>(sym_next) /*&& caller_node->caller_offset == get<1>(sym_next)*/)
                    {
                        auto callee_node = make_shared<ttd_callnode>();

                        callee_node->caller_name = get<0>(sym_next);
                        callee_node->caller_offset = get<1>(sym_next);

                        callee_node->call_event_index = index;
                        callee_node->parent = caller_node;

                        callee_node->func_name = get<0>(sym_insn);

                        callee_node->start_pos.sequence_id = event.position.sequence_id;
                        callee_node->start_pos.step_id = event.position.step_id;

                        caller_node->children.push_back(callee_node);

                        m_ttd_lastnodes[cur_tid] = callee_node;

                        break;
                    }

                    caller_node = caller_node->parent;
                };

                if (caller_node == nullptr)
                {
                    // No last node.

                    caller_node = fix_missing_chain(m_ttd_lastnodes[cur_tid], event.position, sym_next);
                    if (caller_node != nullptr)
                    {
                        auto callee_node = make_shared<ttd_callnode>();

                        callee_node->caller_name = get<0>(sym_next);
                        callee_node->caller_offset = get<1>(sym_next);

                        callee_node->call_event_index = index;
                        callee_node->parent = caller_node;

                        callee_node->func_name = get<0>(sym_insn);

                        callee_node->start_pos.sequence_id = event.position.sequence_id;
                        callee_node->start_pos.step_id = event.position.step_id;

                        caller_node->children.push_back(callee_node);

                        m_ttd_lastnodes[cur_tid] = callee_node;
                    }
                }

#ifdef DEBUG
                stringstream log_ss;
                log_ss << " [ " << hex << setw(4) << index << " ] + "
                    << get<0>(sym_next) << " -> " << get<0>(sym_insn) << endl;
                EXT_F_STR_OUT(log_ss);
#endif
            }
        }

        index++;
    }

    if (total_events != 0)
    {
        for (auto& root : m_ttd_call_forest)
        {
            for (auto& branch : root.second)
            {
                if (branch->call_event_index == (uint64_t)-1)
                    branch->call_event_index = min_index_ttd_callnode(branch);

                if (branch->return_event_index == 0)
                    branch->return_event_index = max_index_ttd_callnode(branch);
            }
        }
    }
}

uint64_t CTTDModelCallstackBuilder::min_index_ttd_callnode(DK_TTD_CALLNODE callnode)
{
    uint64_t min_index = -1;

    for (auto child : callnode->children)
    {
        auto call_index = min_index_ttd_callnode(child);

        if (call_index < min_index)
            min_index = call_index;
    }

    if (callnode->call_event_index < min_index)
        min_index = callnode->call_event_index;
    else
        callnode->call_event_index = min_index;

    if (callnode->call_event_index != -1)
    {
        callnode->start_pos = CTTDInsight::Instance()->m_ttd_events[callnode->call_event_index].position;
    }

    return min_index;
}

uint64_t CTTDModelCallstackBuilder::max_index_ttd_callnode(DK_TTD_CALLNODE callnode)
{
    uint64_t max_index = 0;

    for (auto child : callnode->children)
    {
        auto call_index = max_index_ttd_callnode(child);

        if (call_index > max_index)
            max_index = call_index;
    }

    if (callnode->return_event_index == 0 && callnode->call_event_index != 0 && callnode->call_event_index != -1)
        callnode->return_event_index = callnode->call_event_index;

    if (callnode->return_event_index > max_index)
        max_index = callnode->return_event_index;
    else
        callnode->return_event_index = max_index;

    callnode->end_pos = CTTDInsight::Instance()->m_ttd_events[callnode->return_event_index].position;

    return max_index;
}

