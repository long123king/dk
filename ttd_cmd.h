#pragma once

#include "CmdInterface.h"

#include "ttd/TTD.hpp"

#include "svg/Coordinates.h"
#include "svg/SvgElements.h"

#include <memory>
#include <map>

DECLARE_CMD(ldttd)
DECLARE_CMD(dump_ttd_events)

typedef struct _ttd_position
{
	uint64_t sequence_id;
	uint64_t step_id;
} ttd_position;

typedef struct _ttd_event
{
	enum {
		TTD_NONE_EVENT = 0,
		TTD_CALLRET_EVENT = 1,
		TTD_INDIRECT_JUMP_EVENT = 2,
		TTD_MEM_WATCHPOINT_EVENT = 3,
		TTD_MAX_EVENT,
	} event_type;

	union {
		struct {
			uint64_t insn_addr;
			uint64_t next_addr;
		} callret_event;
		struct {
			uint64_t target_addr;
		} indirect_jump_event;
		struct {
			uint64_t address;
			uint64_t size;
			uint64_t access_type;
		} mem_watchpoint_event;
	}u_data;

	uint64_t tid;
	ttd_position position;
	ttd_position prev_position;
	uint64_t pc;
	uint64_t stack_pointer;
	uint64_t frame_pointer;
}ttd_event;

typedef struct _ttd_callnode
{
	std::string func_name;

	std::string caller_name;
	uint64_t caller_offset;

	uint64_t call_event_index;
	uint64_t return_event_index;

	shared_ptr<struct _ttd_callnode> parent;
	std::vector<shared_ptr<struct _ttd_callnode>> children;

	uint64_t seqs_count{ 0 };
	ttd_position start_pos{ 0,0 };
	ttd_position end_pos{ 0,0 };

	std::string callstack;
}ttd_callnode;

using DK_TTD_CALLNODE = shared_ptr<ttd_callnode>;

class CTTDInsight;

class CTTDModelCallstackBuilder
{
public:
	CTTDModelCallstackBuilder()
	{
	}

	void build_ttd_callstack_model();
	uint64_t min_index_ttd_callnode(DK_TTD_CALLNODE callnode);
	uint64_t max_index_ttd_callnode(DK_TTD_CALLNODE callnode);
	DK_TTD_CALLNODE is_func_in_chain(std::tuple<std::string, uint64_t> func, DK_TTD_CALLNODE chain);

	std::tuple<std::string, uint64_t> addr2sym(uint64_t addr);
	DK_TTD_CALLNODE fix_missing_chain(DK_TTD_CALLNODE root_chain, ttd_position pos, std::tuple<std::string, uint64_t> orphan_event_node);

	void dump_root_chain(DK_TTD_CALLNODE root_chain);

	std::tuple<DK_TTD_CALLNODE, DK_TTD_CALLNODE> build_new_root_chain(ttd_position pos);

	void build_callstack_svg(std::string filename, DK_TTD_CALLNODE root);

	void append_callstack_svg_node(DK_TTD_CALLNODE node, CSvgPoint pivot);

	void calc_seqs_count(DK_TTD_CALLNODE node);

	std::map<uint64_t, DK_TTD_CALLNODE> m_ttd_lastnodes;
	std::map<uint64_t, std::vector<DK_TTD_CALLNODE>> m_ttd_call_forest;

	std::vector<std::string> m_root_chain;

	shared_ptr<CSvgGroup> m_rect_g;
	shared_ptr<CSvgGroup> m_text_g;
	shared_ptr<CSvgGroup> m_tag_g;

	uint64_t m_canvas_width{ 0 };
	uint64_t m_canvas_height{ 0 };

	uint64_t m_canvas_lines{ 1 };
};

class CTTDInsight
{
	friend class CTTDModelCallstackBuilder;
public:
	static CTTDInsight* Instance()
	{
		static CTTDInsight s_instance;
		return &s_instance;
	}

	void ttd_call_ret_callback(
		uintptr_t context,
		TTD::GuestAddress guestInstructionAddress,
		TTD::GuestAddress guestFallThroughInstructionAddress,
		TTD::TTD_Replay_IThreadView* threadView);

	void load_ttd();

	void dump_ttd_model_events(std::string out_filename);

	std::string dump_ttd_callnode(DK_TTD_CALLNODE callnode, size_t level = 0);

protected:
	std::vector<ttd_event> m_ttd_events;

	std::vector<DK_TTD_CALLNODE> m_ttd_call_forest;
};

