#include "model.h"

#include "CmdExt.h"
#include "CmdList.h"
#include <iomanip>
#pragma warning( disable : 4311)
#pragma warning( disable : 4302)

LOG_DEFINE_MGR

DEFINE_CMD(ls_model)
{
    if (args.size() < 2)
    {
        EXT_F_ERR("Usage: !dk ls_model <model_path>\n");
        return;
    }
    auto results = DK_DUMP(mobj)(DK_MODEL_ACCESS->get_mobj_tree(args[1]), true, args[1]);

    EXT_F_DML(results.c_str());
}

DEFINE_CMD(exec)
{
    if (args.size() < 2)
    {
        EXT_F_ERR("Usage: !dk exec <command>\n");
        return;
    }

    auto command = args[1];

    auto results = DK_X_CMD(command);
    stringstream ss;

    ss << "Execute command: " << command << endl;
    for (auto result : results)
    {
        ss << result << endl;
    }

    EXT_F_STR_OUT(ss);
}

DEFINE_CMD(ls_sessions)
{
    size_t i = 0;
    for (auto& session : DK_MODEL_ACCESS->m_sessions)
    {
        stringstream ss;
        ss << hex << " Session [ " << get<0>(session) << " ] "
            << endl
            << DK_DUMP(session)(get<1>(session));

        EXT_F_STR_OUT(ss);
    }
}

DEFINE_CMD(mobj)
{
    if (args.size() < 2)
    {
        EXT_F_ERR("Usage: !dk mobj <mobj_path>\n");
        return;
    }

    auto mobj_path = args[1];

    auto mobj = DK_MODEL_ACCESS->get_mobj_tree(mobj_path);

    EXT_F_DML(DK_DUMP(mobj)(mobj, true, mobj_path).c_str());
}

DEFINE_CMD(mobj_at)
{
    if (args.size() < 3)
    {
        EXT_F_ERR("Usage: !dk mobj_at <mobj_path> <index>\n");
        return;
    }

    auto mobj_path = args[1];
    auto index = EXT_F_IntArg(args, 2, 0);

    auto mobj = DK_MODEL_ACCESS->get_mobj_tree(mobj_path);

    auto child_obj = DK_MODEL_ACCESS->at(mobj, index);

    stringstream ss;

    ss << mobj_path << "[" << index << "]";

    EXT_F_DML(DK_DUMP(mobj)(child_obj, true, ss.str()).c_str());
}

DEFINE_CMD(call)
{
    if (args.size() < 2)
    {
        EXT_F_ERR("Usage: !dk call <mobj_path>\n");
        return;
    }

    auto mobj_path = args[1];

    auto mobj = DK_MODEL_ACCESS->get_mobj_tree(mobj_path);

    auto parent_path = mobj_path;
    auto it_last = parent_path.find_last_of('.');
    if (it_last != parent_path.npos)
        parent_path.resize(it_last);

    auto parent_mobj = DK_MODEL_ACCESS->get_mobj_tree(parent_path);

    EXT_F_DML(DK_DUMP(mobj)(mobj, true, mobj_path).c_str());

    vector<DK_MOBJ_PTR> vec_args;
    auto call_result = DK_MODEL_ACCESS->call(mobj, parent_mobj, vec_args);
    if (call_result)
        EXT_F_DML(DK_DUMP(mobj)(call_result, true, mobj_path).c_str());
}

DEFINE_CMD(ccall)
{
    if (args.size() < 3)
    {
        EXT_F_ERR("Usage: !dk call <mobj_path> <context_path>\n");
        return;
    }

    auto mobj_path = args[1];

    auto mobj = DK_MODEL_ACCESS->get_mobj_tree(mobj_path);

    auto parent_path = args[2];

    auto parent_mobj = DK_MODEL_ACCESS->get_mobj_tree(parent_path);

    EXT_F_DML(DK_DUMP(mobj)(mobj, true, mobj_path).c_str());

    vector<DK_MOBJ_PTR> vec_args;
    auto call_result = DK_MODEL_ACCESS->call(mobj, parent_mobj, vec_args);
    if (call_result)
        EXT_F_DML(DK_DUMP(mobj)(call_result, true, mobj_path).c_str());
}

DEFINE_CMD(ls_processes)
{
    if (args.size() < 2)
    {
        EXT_F_ERR("Usage: !dk ls_processes <session_id>\n");
        return;
    }

    auto session_id = EXT_F_IntArg(args, 1, 0);
    
    auto session = DK_MGET_SESN(session_id);

    stringstream ss;
    ss << hex << " Session [ " << session_id << " ] "
        << endl;

    auto pobj_processes = DK_MGET_POBJ(session, "Processes");
    if (pobj_processes != nullptr)
    {
        auto processes = DK_MODEL_ACCESS->iterate(pobj_processes);

        for (auto& proc : processes)
        {
            ss << " [ " << get<0>(proc) << " ] " << DK_DUMP(process)(get<1>(proc)).c_str();
        }
    }

    EXT_F_STR_OUT(ss);
}

DEFINE_CMD(ls_threads)
{
    if (args.size() < 3)
    {
        EXT_F_OUT("Usage: !dk ls_threads <session_id> <process_id>\n");
        return;
    }
    
    auto session_id = EXT_F_IntArg(args, 1, 0);
    auto proc_id = EXT_F_IntArg(args, 2, 0);

    stringstream ss;

    ss << hex << " threads for session [" << session_id
        << "] process [" << proc_id
        << "]:" << endl;

    auto session = DK_MGET_SESN(session_id);
    if (session != nullptr)
    {
        auto proc = DK_MGET_PROC(session_id, proc_id);
        if (proc != nullptr)
        {
            auto pobj_threads = DK_MGET_POBJ(proc, "Threads");
            auto threads = DK_MODEL_ACCESS->iterate(pobj_threads);

            for (auto& thread : threads)
            {
                ss << " [ " << get<0>(thread) << " ] "
                    << DK_DUMP(thread)(get<1>(thread));
            }
        }
    }

    EXT_F_STR_OUT(ss);
}

DEFINE_CMD(ls_modules)
{
    if (args.size() < 3)
    {
        EXT_F_OUT("Usage: !dk ls_modules <session_id> <process_id>\n");
        return;
    }
    
    auto session_id = EXT_F_IntArg(args, 1, 0);
    auto proc_id = EXT_F_IntArg(args, 2, 0);

    stringstream ss;

    ss << hex << " modules for session [" << session_id
        << "] process [" << proc_id
        << "]:" << endl;

    auto session = DK_MGET_SESN(session_id);
    if (session != nullptr)
    {
        auto proc = DK_MGET_PROC(session_id, proc_id);
        if (proc != nullptr)
        {
            auto pobj_modules = DK_MGET_POBJ(proc, "Modules");
            auto modules = DK_MODEL_ACCESS->iterate(pobj_modules);

            for (auto& module : modules)
            {
                auto module_info = DK_MGET_MODL(get<1>(module));

                ss << " [ " << get<0>(module)
                    << " ] " << DK_DUMP(module)(get<1>(module));
            }
        }
    }
    EXT_F_STR_OUT(ss);
}

DEFINE_CMD(ls_handles)
{
    if (args.size() < 3)
    {
        EXT_F_OUT("Usage: !dk ls_handles <session_id> <process_id>\n");
        return;
    }

    auto session_id = EXT_F_IntArg(args, 1, 0);
    auto proc_id = EXT_F_IntArg(args, 2, 0);

    stringstream ss;

    ss << hex << " handles for session [" << session_id
        << "] process [" << proc_id
        << "]:" << endl;

    auto session = DK_MGET_SESN(session_id);
    if (session != nullptr)
    {
        auto proc = DK_MGET_PROC(session_id, proc_id);
        if (proc != nullptr)
        {
            auto io = DK_MGET_POBJ(proc, "Io");
            if (io != nullptr)
            {
                auto pobj_handles = DK_MGET_POBJ(io, "Handles");
                if (pobj_handles != nullptr)
                {
                    auto handles = DK_MODEL_ACCESS->iterate(pobj_handles);

                    for (auto& handle : handles)
                    {
                        ss << " [ " << get<0>(handle)
                            << " ] " << DK_DUMP(handle)(get<1>(handle));
                    }
                }
            }
        }
    }
    EXT_F_STR_OUT(ss);
}

DEFINE_CMD(ps_ttd)
{
    if (!DK_MODEL_ACCESS->isTTD() || args.size() < 3)
    {
        EXT_F_OUT("Usage: !dk ps_ttd <session_id> <pid>\nTTD Mode Only\n");
        return;
    }

    auto session_id = EXT_F_IntArg(args, 1, 0);
    auto proc_id = EXT_F_IntArg(args, 2, 0);

    auto proc = DK_MGET_PROC(session_id, proc_id);
    if (proc != nullptr)
    {
        auto ttd = DK_MGET_POBJ(proc, "TTD");
        if (ttd != nullptr)
        {
            stringstream ss;
            auto lifetime = DK_MGET_POBJ(ttd, "Lifetime");
            auto min_pos = DK_MGET_POS(lifetime, "MinPosition");
            auto max_pos = DK_MGET_POS(lifetime, "MaxPosition");

            DK_MODEL_ACCESS->seek_to(get<0>(min_pos), get<1>(min_pos));

            auto events_pobj = DK_MGET_POBJ(ttd, "Events");
            auto events = DK_MODEL_ACCESS->iterate(events_pobj);
            for (auto& event : events)
            {
                ss << " [ " << setw(3) << get<0>(event)
                    << " ] " << DK_DUMP(event)(get<1>(event));
            }
            EXT_F_STR_OUT(ss);
        }
    }
}

DEFINE_CMD(session_ttd)
{
    if (!DK_MODEL_ACCESS->isTTD() || args.size() < 2)
    {
        EXT_F_OUT("Usage: !dk session_ttd <session_id>\nTTD Mode Only\n");
        return;
    }

    auto session_id = EXT_F_IntArg(args, 1, 0);

    auto session = DK_MGET_SESN(session_id);
    if (session != nullptr)
    {
        auto ttd = DK_MGET_POBJ(session, "TTD");
        if (ttd != nullptr)
        {
            stringstream ss;
            auto resources = DK_MGET_POBJ(ttd, "Resources");
            auto heap_memory_pobj = DK_MGET_POBJ(resources, "HeapMemory");
            
            auto heap_memories = DK_MODEL_ACCESS->iterate(heap_memory_pobj);
            for (auto& heap_memory : heap_memories)
            {
                ss << hex << " [ " << get<0>(heap_memory)
                    << " ] " << DK_DUMP(heap_memory)(get<1>(heap_memory));
            }
            EXT_F_STR_OUT(ss);
        }
    }
}

DEFINE_CMD(ttd_calls)
{
    if (!DK_MODEL_ACCESS->isTTD() || args.size() < 3)
    {
        EXT_F_OUT("Usage: !dk ttd_calls <session_id> <call_pattern>\nTTD Mode Only\n");
        return;
    }

    auto session_id = EXT_F_IntArg(args, 1, 0);
    auto call_pattern = args[2];

    auto session = DK_MGET_SESN(session_id);
    if (session != nullptr)
    {
        auto ttd = DK_MGET_POBJ(session, "TTD");
        if (ttd != nullptr)
        {
            auto calls_pobj = DK_MGET_POBJ(ttd, "Calls");

            if (calls_pobj != nullptr)
            {
                auto arg = DK_MODEL_ACCESS->create_str_intrinsic_obj(call_pattern);

                vector<DK_MOBJ_PTR> vec_args;
                vec_args.push_back(arg);

                auto call_result = DK_MODEL_ACCESS->call(calls_pobj, ttd, vec_args);
                if (call_result != nullptr)
                {
                    auto results = DK_MODEL_ACCESS->iterate(call_result);
                    for (auto& result : results)
                    {                         
                        EXT_F_OUT(DK_DUMP(call_result)(get<1>(result)).c_str());
                    }
                    LOG_UNDEF_MGR
                }
            }
        }
    }
}

DEFINE_CMD(ttd_mem_access)
{
    if (!DK_MODEL_ACCESS->isTTD() || args.size() < 5)
    {
        EXT_F_OUT("Usage: !dk ttd_mem_access <session_id> <start_addr> <end_addr> <mode:R/W/E/C>\nTTD Mode Only\n");
        return;
    }

    auto session_id = EXT_F_IntArg(args, 1, 0);
    auto start_addr = EXT_F_IntArg(args, 2, 0);
    auto end_addr = EXT_F_IntArg(args, 3, 0);
    auto mode = args[4];
    
    auto session = DK_MGET_SESN(session_id);
    if (session != nullptr)
    {
        auto ttd = DK_MGET_POBJ(session, "TTD");
        if (ttd != nullptr)
        {
            auto mem_pobj = DK_MGET_POBJ(ttd, "Memory");

            if (mem_pobj != nullptr)
            {
                auto arg_start_addr = DK_MODEL_ACCESS->create_int_intrinsic_obj<uint64_t, VT_UI8>(start_addr);
                auto arg_end_addr = DK_MODEL_ACCESS->create_int_intrinsic_obj<uint64_t, VT_UI8>(end_addr);
                auto arg_mode = DK_MODEL_ACCESS->create_str_intrinsic_obj(mode);

                vector<DK_MOBJ_PTR> vec_args;
                vec_args.push_back(arg_start_addr);
                vec_args.push_back(arg_end_addr);
                vec_args.push_back(arg_mode);

                auto mem_access_result = DK_MODEL_ACCESS->call(mem_pobj, ttd, vec_args);
                if (mem_access_result != nullptr)
                {
                    auto results = DK_MODEL_ACCESS->iterate(mem_access_result);
                    for (auto& result : results)
                    {
                        EXT_F_OUT(DK_DUMP(mem_access_result)(get<1>(result)).c_str());
                    }
                    LOG_UNDEF_MGR
                }
            }
        }
    }
}

DEFINE_CMD(ttd_mem_use)
{
    if (!DK_MODEL_ACCESS->isTTD() || args.size() < 6)
    {
        EXT_F_OUT("Usage: !dk ttd_mem_use <session_id> <proc_id> <thread_id> <start_percent> <end_percent> \nTTD Mode Only\n");
        return;
    }

    auto session_id = EXT_F_IntArg(args, 1, 0);
    auto proc_id = EXT_F_IntArg(args, 2, 0);
    auto thread_id = EXT_F_IntArg(args, 3, 0);
    auto start_percent = EXT_F_IntArg(args, 4, 0);
    auto end_percent = EXT_F_IntArg(args, 5, 0);

    //auto thread = DK_MGET_THRD(session_id, proc_id, thread_id);
    auto thread = DK_MGET_PROC(session_id, proc_id);
    if (thread != nullptr)
    {
        auto ttd = DK_MGET_POBJ(thread, "TTD");
        if (ttd != nullptr)
        {
            auto mem_use_pobj = DK_MGET_POBJ(ttd, "GatherMemoryUse");

            if (mem_use_pobj != nullptr)
            {
                auto arg_start_percent = DK_MODEL_ACCESS->create_int_intrinsic_obj<uint64_t, VT_UI8>(start_percent);
                auto arg_end_percent = DK_MODEL_ACCESS->create_int_intrinsic_obj<uint64_t, VT_UI8>(end_percent);

                vector<DK_MOBJ_PTR> vec_args;
                vec_args.push_back(arg_start_percent);
                vec_args.push_back(arg_end_percent);

                auto mem_use_result = DK_MODEL_ACCESS->call(mem_use_pobj, ttd, vec_args);
                if (mem_use_result != nullptr)
                {
                    auto results = DK_MODEL_ACCESS->iterate(mem_use_result);
                    for (auto& result : results)
                    {
                        EXT_F_DML(DK_DUMP(mem_use_result)(get<1>(result)).c_str());
                        break;
                    }
                    LOG_UNDEF_MGR
                }
            }
        }
    }
}

DEFINE_CMD(cur_context)
{
    auto cur_session = DK_MODEL_ACCESS->get_current_session();
    auto cur_process = DK_MODEL_ACCESS->get_current_process();
    auto cur_thread = DK_MODEL_ACCESS->get_current_thread();
    auto cur_stack = DK_MODEL_ACCESS->get_current_stack();
    auto cur_frame = DK_MODEL_ACCESS->get_current_frame();

    EXT_F_OUT("current session: 0x%0I64x, \n\t%s", cur_session.Get(), DK_DUMP(session)(cur_session).c_str());
    EXT_F_OUT("current process: 0x%0I64x, \n\t%s", cur_process.Get(), DK_DUMP(process)(cur_process).c_str());
    EXT_F_OUT("current thread: 0x%0I64x, \n\t%s", cur_thread.Get(), DK_DUMP(thread)(cur_thread).c_str());
    EXT_F_OUT("current stack: 0x%0I64x, \n\t%s", cur_stack.Get(), DK_DUMP(stack)(cur_stack).c_str());
    EXT_F_OUT("current frame: 0x%0I64x, \n\t%s", cur_frame.Get(), DK_DUMP(frame)(cur_frame).c_str());
}

// {F2BCE54E-4835-4f8a-836E-7981E29904D1}
const GUID IID_IHostDataModelAccess = { 0xf2bce54e, 0x4835, 0x4f8a, { 0x83, 0x6e, 0x79, 0x81, 0xe2, 0x99, 0x4, 0xd1 } };

map<ModelObjectKind, string> CModelAccess::s_map_kind_name{
    { ObjectPropertyAccessor,       "PropertyAccessor"},
    { ObjectContext,                "Context"},
    { ObjectTargetObject,           "TargetObject"},
    { ObjectTargetObjectReference,  "TargetObjectReference"},
    { ObjectSynthetic,              "Synthetic"},
    { ObjectNoValue,                "NoValue"},
    { ObjectError,                  "Error"},
    { ObjectIntrinsic,              "Intrinsic"},
    { ObjectMethod,                 "Method"},
    { ObjectKeyReference,           "KeyReference"}
};

map<VARTYPE, string> CModelAccess::s_map_vt_name{
    { VT_EMPTY,     "VT_EMPTY"},
    { VT_NULL,      "VT_NULL"},
    { VT_I2,        "VT_I2"},
    { VT_I4,        "VT_I4"},
    { VT_R4,        "VT_R4"},
    { VT_R8,        "VT_R8"},
    { VT_CY,        "VT_CY"},
    { VT_DATE,      "VT_DATE"},
    { VT_BSTR,      "VT_BSTR"},
    { VT_DISPATCH,  "VT_DISPATCH"},
    { VT_ERROR,     "VT_ERROR"},
    { VT_BOOL,      "VT_BOOL"},
    { VT_VARIANT,   "VT_VARIANT"},
    { VT_UNKNOWN,   "VT_UNKNOWN"},
    { VT_DECIMAL,   "VT_DECIMAL"},
    { VT_RECORD,    "VT_RECORD"},
    { VT_I1,        "VT_I1"},
    { VT_UI1,       "VT_UI1"},
    { VT_UI2,       "VT_UI2"},
    { VT_UI4,       "VT_UI4"},
    { VT_INT,       "VT_INT"},
    { VT_UINT,      "VT_UINT"},
    { VT_ARRAY,     "VT_ARRAY"},
    { VT_BYREF,     "VT_BYREF"}
};

string BSTR2str(BSTR bstr)
{
    if (bstr == nullptr)
        return "";

    wstring wstr(bstr);
    return string(wstr.begin(), wstr.end());
}

CModelAccess::CModelAccess()
{
    HRESULT hr = EXT_D_IDebugClient->QueryInterface(IID_IHostDataModelAccess, (PVOID*)&m_model_access);
    if (S_OK == hr)
    {
        hr = m_model_access->GetDataModel(&m_model_mgr, &m_debug_host);

        if (S_OK == hr)
        {
            visit_root();
        }
    }
}

string CModelAccess::dump_frame(DK_MOBJ_PTR frame)
{
    stringstream ss;
    auto frame_attrs = get_pobj(frame, "Attributes");
    if (frame_attrs != nullptr)
    {
        auto insn_ptr = get_pvalue<uint64_t, VT_UI8>(frame_attrs, "InstructionOffset");
        auto return_ptr = get_pvalue<uint64_t, VT_UI8>(frame_attrs, "ReturnOffset");
        auto frame_ptr = get_pvalue<uint64_t, VT_UI8>(frame_attrs, "FrameOffset");
        auto stack_ptr = get_pvalue<uint64_t, VT_UI8>(frame_attrs, "StackOffset");
        auto ftable_entry = get_pvalue<uint64_t, VT_UI8>(frame_attrs, "FunctionTableEntry");
        auto is_virtual = get_pvalue<int, VT_I4>(frame_attrs, "Virtual");
        auto frame_number = get_pvalue<DWORD, VT_UI4>(frame_attrs, "FrameNumber");

        ss << hex << "#" << setw(3) << frame_number
            << " Frame: 0x" << setw(16) << frame_ptr
            << " Stack: 0x" << setw(16) << stack_ptr
            << " Return: 0x" << setw(16) << return_ptr
            << " Insn: 0x" << setw(16) << insn_ptr
            /*<< " fte:" << ftable_entry*/;

        auto symbol = EXT_F_Addr2Sym(insn_ptr);

        //if (is_virtual)
        //    ss << " Virtual ";

        ss << " (" << get<0>(symbol) << "+" << get<1>(symbol) << ")" << endl;
    }

    return ss.str();
}

string CModelAccess::dump_thread(DK_MOBJ_PTR thread)
{
    stringstream ss;

    auto tid = get_pvalue<uint64_t, VT_UI8>(thread, "Id");

    ss << " Thread Id: " << tid << endl;

    auto stack = get_pobj(thread, "Stack");
    if (stack != nullptr)
    {
        auto frames_mobj = get_pobj(stack, "Frames");
        if (frames_mobj != nullptr)
        {
            auto frames = iterate(frames_mobj);
            for (auto& frame : frames)
            {
                ss << dump_frame(get<1>(frame));
            }
        }
    }

    return ss.str();
}

string CModelAccess::dump_process(DK_MOBJ_PTR process)
{
    stringstream ss;

    auto proc_name = BSTR2str(get_pvalue<BSTR, VT_BSTR>(process, "Name"));
    auto pid = get_pvalue<uint64_t, VT_UI8>(process, "Id");

    ss << string(100, '-') << endl
        << " Name: " << setw(30) << proc_name << " , "
        << ", Id: " << pid << endl;

    return ss.str();
}

string CModelAccess::dump_session(DK_MOBJ_PTR session)
{
    stringstream ss;

    auto session_id = get_pvalue<uint64_t, VT_UI8>(session, "Id");

    ss << " Id: " << session_id << endl;

    if (isNT())
        ss << " NT ";

    if (isLive())
        ss << " Live ";

    if (isUsermode())
        ss << " Usermode ";

    if (isKernelmode())
        ss << " Kernelmode ";

    if (isTTD())
        ss << " TTD ";

    if (isDump())
        ss << " Dump ";

    if (isTTD() || isDump())
    {
        ss << "(" << dumpFilename() << ")";
    }

    ss << endl;
    return ss.str();
}

string CModelAccess::dump_stack(DK_MOBJ_PTR stack)
{
    stringstream ss;

    auto frame_pobj = get_pobj(stack, "Frames");

    if (frame_pobj != nullptr)
    {
        auto frames = iterate(frame_pobj);

        ss << "Frames:\n";
        for (auto& frame : frames)
        {
            ss << DK_DUMP(frame)(get<1>(frame));
        }
    }

    return ss.str();
}

string CModelAccess::dump_current_callstack()
{
    auto cur_stack = DK_MODEL_ACCESS->get_current_stack();

    return dump_stack(cur_stack);
}

string CModelAccess::dump_call_result(DK_MOBJ_PTR call_result)
{
    stringstream ss;
    auto start_pos = get_pos(call_result, "TimeStart");
    auto end_pos = get_pos(call_result, "TimeEnd");

    auto function = BSTR2str(get_pvalue<BSTR, VT_BSTR>(call_result, "Function"));
    auto func_addr = get_pvalue<uint64_t, VT_UI8>(call_result, "FunctionAddress");
    auto return_addr = get_pvalue<uint64_t, VT_UI8>(call_result, "ReturnAddress");
    auto return_val = get_pvalue<uint64_t, VT_UI8>(call_result, "ReturnValue");
    auto thread_id = get_pvalue<uint64_t, VT_UI8>(call_result, "ThreadId");
    auto event_type = get_pvalue<uint64_t, VT_UI8>(call_result, "EventType");

    ss
        << hex
        << " (" << get<0>(start_pos) << " : " << get<1>(start_pos) << ") - "
        << " (" << get<0>(end_pos) << " : " << get<1>(end_pos) << ") "
        << " event_type: " << event_type
        << " thread_id: " << thread_id
        << endl
        << " addr: 0x" << setw(16) << func_addr
        << " ret_addr: 0x" << setw(16) << return_addr
        << " ret_val: 0x" << setw(16) << return_val
        << " function: " << function
        ;

    auto parameters_pobj = get_pobj(call_result, "Parameters");
    if (parameters_pobj != nullptr)
    {
        ss << " parameters: " << endl;
        auto params = iterate(parameters_pobj);
        for (auto& param : params)
        {
            ss << " [ " << get<0>(param) << " ] "
                << get_value<uint64_t, VT_UI8>(get<1>(param)) << endl;
        }

        params.clear();
    }

    ss << endl;

    return ss.str();
}

string CModelAccess::dump_mem_access_result(DK_MOBJ_PTR mem_access_result)
{
    stringstream ss;
    auto start_pos = get_pos(mem_access_result, "TimeStart");
    auto end_pos = get_pos(mem_access_result, "TimeEnd");

    auto access_type = BSTR2str(get_pvalue<BSTR, VT_BSTR>(mem_access_result, "AccessType"));
    auto ip_addr = get_pvalue<uint64_t, VT_UI8>(mem_access_result, "IP");
    auto addr = get_pvalue<uint64_t, VT_UI8>(mem_access_result, "Address");
    auto size = get_pvalue<uint64_t, VT_UI8>(mem_access_result, "Size");
    auto value = get_pvalue<uint64_t, VT_UI8>(mem_access_result, "Value");
    auto overwritten_value = get_pvalue<uint64_t, VT_UI8>(mem_access_result, "OverwrittenValue");
    auto thread_id = get_pvalue<uint64_t, VT_UI8>(mem_access_result, "ThreadId");
    auto event_type = get_pvalue<uint64_t, VT_UI8>(mem_access_result, "EventType");

    ss
        << hex
        << " (" << get<0>(start_pos) << " : " << get<1>(start_pos) << ") - "
        << " (" << get<0>(end_pos) << " : " << get<1>(end_pos) << ") "
        << " event_type: " << event_type
        << " thread_id: " << thread_id
        << endl
        << " IP: 0x" << setw(16) << ip_addr
        << " addr: 0x" << setw(16) << addr
        << " size: 0x" << setw(16) << size
        << " value: 0x" << setw(16) << value
        << " new_value: 0x" << setw(16) << overwritten_value
        << " access type: " << access_type
        ;

    auto seek_to_pobj = get_pobj_tree(mem_access_result, "TimeStart.SeekTo");
    auto seek_to_result = DK_MODEL_ACCESS->call(seek_to_pobj, mem_access_result, vector<DK_MOBJ_PTR>());

    auto symbol = EXT_F_Addr2Sym(ip_addr);

    ss << " (" << get<0>(symbol) << "+" << get<1>(symbol) << ")" << endl;

    auto cur_stack = DK_MODEL_ACCESS->get_current_stack();
    ss << "current stack: " << cur_stack.Get() << ", \n\t" << DK_DUMP(stack)(cur_stack);

    ss << endl;

    return ss.str();
}

string CModelAccess::dump_mem_use_result(DK_MOBJ_PTR mem_use_result)
{
    stringstream ss;

    vector<string> use_kinds = {
        "CodeRanges",
        "InputRanges",
        "OutputRanges",
        "TotalInputRanges",
        "TotalRanges",
        "TouchedRanges",
        "RegisterMismatchList",
        "DataMismatchList",
        "InputCacheLineRanges",
    };

    for (auto use_kind : use_kinds)
    {
        auto code_range_pobj = get_pobj(mem_use_result, use_kind);
        if (code_range_pobj != nullptr)
        {
            auto code_ranges = iterate(code_range_pobj);

            ss << use_kind << " : " << endl;
            for (auto code_range : code_ranges)
            {
                //ss << dump_mobj(get<1>(code_range)) << endl;

                auto addr_range = get_pobj(get<1>(code_range), "AddressRange");

                if (addr_range != nullptr)
                {
                    auto min_addr = get_pvalue<uint64_t, VT_UI8>(addr_range, "MinAddress");
                    auto max_addr = get_pvalue<uint64_t, VT_UI8>(addr_range, "MaxAddress");

                    //auto data = get_pobj(get<1>(code_range), "Data");

                    auto code_sym = EXT_F_Addr2Sym(min_addr);

                    ss << hex << setfill('0')
                        << " 0x" << setw(16) << min_addr << " - 0x" << max_addr
                        << " ( " << setw(5) << setfill(' ') << max_addr - min_addr << " ) "
                        //<< get<0>(code_sym) << "+0x" << get<1>(code_sym)
                        << endl;

                }
                else
                {
                    auto min_addr = get_pvalue<uint64_t, VT_UI8>(get<1>(code_range), "MinAddress");
                    auto max_addr = get_pvalue<uint64_t, VT_UI8>(get<1>(code_range), "MaxAddress");

                    ss << hex << setfill('0')
                        << " 0x" << setw(16) << min_addr << " - 0x" << max_addr
                        << " ( " << setw(5) << setfill(' ') << max_addr - min_addr << " ) "
                        //<< get<0>(code_sym) << "+0x" << get<1>(code_sym)
                        << endl;
                }

            }
        }
    }

    return ss.str();
}

vector<array<uint64_t, 5>> CModelAccess::get_current_callstack()
{
    vector<array<uint64_t, 5>> vec_results;

    auto cur_stack_frame = get_pobj_tree(m_debugger, "State.DebuggerVariables.curstack.Frames");

    if (cur_stack_frame != nullptr)
    {
        auto frames = iterate(cur_stack_frame);

        for (auto& frame : frames)
        {
            auto frame_attrs = get_pobj(get<1>(frame), "Attributes");
            if (frame_attrs != nullptr)
            {
                auto frame_number = get_pvalue<DWORD, VT_UI4>(frame_attrs, "FrameNumber");
                auto insn_ptr = get_pvalue<uint64_t, VT_UI8>(frame_attrs, "InstructionOffset");
                auto return_ptr = get_pvalue<uint64_t, VT_UI8>(frame_attrs, "ReturnOffset");
                auto frame_ptr = get_pvalue<uint64_t, VT_UI8>(frame_attrs, "FrameOffset");
                auto stack_ptr = get_pvalue<uint64_t, VT_UI8>(frame_attrs, "StackOffset");

                vec_results.emplace_back(array<uint64_t, 5>({ frame_number, insn_ptr, return_ptr, frame_ptr, stack_ptr }));
            }
        }
    }

    return vec_results;
}

void CModelAccess::seek_to(uint64_t seq, uint64_t step)
{
    if (!isTTD())
        return;

    stringstream ss;

    ss << "!ttdext.tt " << hex << seq << ":" << step;

    execute_cmd(ss.str());

    auto cur_thread_ttd = DK_MODEL_ACCESS->get_pobj_tree(DK_MODEL_ACCESS->m_debugger, "State.DebuggerVariables.curthread.TTD");

    if (cur_thread_ttd != nullptr)
    {
        auto cur_pos = get_pos(cur_thread_ttd, "Position");

        if (seq != get<0>(cur_pos) || step != get<1>(cur_pos))
        {
            EXT_F_OUT("Fail to SeekTo (%x:%x), current position: (%x:%x)\n", seq, step, get<0>(cur_pos), get<1>(cur_pos));
        }
        else
        {
            //EXT_F_OUT("Succeed in seeking to (%x:%x)\n", seq, step);
        }
    }
}

string CModelAccess::dump_mobj(DK_MOBJ_PTR mobj, bool b_show_children, string mobj_path)
{
    stringstream ss;
    auto kind = get_kind(mobj);
    ss << "IModelObject kind: " << s_map_kind_name[kind];

    if (ObjectIntrinsic == kind)
    {
        VARIANT the_value;

        auto hr = mobj->GetIntrinsicValue(&the_value);
        if (S_OK == hr)
        {
            auto vt_type = s_map_vt_name[the_value.vt];

            ss << "\tIntrinsic type: " << vt_type << "\t";

            if (the_value.vt == VT_BSTR)
            {
                auto mobj_str = BSTR2str(the_value.bstrVal);

                ss << "BSTR value: " << mobj_str;
            }
            else
            {
                ss << "int value: 0x" << hex << setw(16) << setfill('_') << the_value.ullVal;
            }
        }

        VariantClear(&the_value);
    }

    ss << endl;

    if (b_show_children)
    {
        ss << "\tChildren: " << endl;

        size_t index = 0;
        for (auto kv : enum_keyvalues(mobj))
        {
            ss << "\t\t #[" << index++ << "] ";                

            if (get<1>(kv) == "Intrinsic")
            {
                VARIANT the_value;

                auto hr = get<2>(kv)->GetIntrinsicValue(&the_value);
                if (S_OK == hr)
                {
                    ss << setfill(' ') << setw(30) << get<0>(kv) << " Intrinsic type: " << get<1>(kv) << "\t";

                    if (the_value.vt == VT_BSTR)
                    {
                        auto mobj_str = BSTR2str(the_value.bstrVal);

                        ss << "BSTR value: " << mobj_str;
                    }

                    else
                    {
                        ss << "int value: 0x" << hex << setw(16) << setfill('_') << the_value.ullVal;
                    }
                }
                VariantClear(&the_value);
            }
            else if (get<1>(kv) == "Method")
            {
                ss << setfill(' ')
                    << DML_CMD << "!dk call " << mobj_path << "." << get<0>(kv)
                    << DML_TEXT << setfill(' ') << setw(30) << get<0>(kv)
                    << DML_END
                    << " "
                    << get<1>(kv);
            }
            else
            {
                ss << setfill(' ')
                    << DML_CMD << "!dk mobj " << mobj_path << "." << get<0>(kv)
                    << DML_TEXT << setfill(' ') << setw(30) << get<0>(kv)
                    << DML_END
                    << " "
                    << get<1>(kv);
            }


            ss << endl;
        }

        auto iter_results = iterate(mobj);
        for (auto iter_result : iter_results)
        {
            auto kind = get_kind(get<1>(iter_result));

            ss << "child "; 
            //results.push_back(make_tuple(ss.str(), s_map_kind_name[kind], "", get<1>(iter_result)));

            ss << setfill(' ')
                << DML_CMD << "!dk mobj_at " << mobj_path << " " << hex << get<0>(iter_result)
                << DML_TEXT << setfill(' ')  << "#[ " << hex << get<0>(iter_result) << " ] (" << s_map_kind_name[kind] << ") "
                << DML_END
                << " ";
        }
    }

    return ss.str();
}

vector<string> CModelAccess::execute_cmd(string command)
{
    vector<string> vec_results;

    auto x_mobj = DK_MODEL_ACCESS->get_pobj_tree(DK_MODEL_ACCESS->m_debugger, "Utility.Control.ExecuteCommand");
    if (x_mobj != nullptr)
    {
        auto arg = DK_MODEL_ACCESS->create_str_intrinsic_obj(command);

        vector<DK_MOBJ_PTR> vec_args;
        vec_args.push_back(arg);

        auto call_result = DK_MODEL_ACCESS->call(x_mobj, x_mobj, vec_args);
        if (call_result != nullptr)
        {
            auto results = DK_MODEL_ACCESS->iterate(call_result);
            for (auto& result : results)
            {
                vec_results.push_back(BSTR2str(get_value<BSTR, VT_BSTR>(get<1>(result))));
            }
        }
    }

    return vec_results;
}

uint64_t CModelAccess::get_current_tid()
{
    auto cur_thread = DK_MODEL_ACCESS->get_current_thread();

    auto cur_tid = get_pvalue<uint64_t, VT_UI8>(cur_thread, "Id");

    return cur_tid;
}

vector<ttd_mem_access> CModelAccess::get_mem_access(uint64_t start_addr, uint64_t end_addr, string mode)
{
    vector<ttd_mem_access> mem_accesses;
    auto session = get_current_session();
    if (session != nullptr)
    {
        auto ttd = DK_MGET_POBJ(session, "TTD");
        if (ttd != nullptr)
        {
            auto mem_pobj = DK_MGET_POBJ(ttd, "Memory");

            if (mem_pobj != nullptr)
            {
                auto arg_start_addr = DK_MODEL_ACCESS->create_int_intrinsic_obj<uint64_t, VT_UI8>(start_addr);
                auto arg_end_addr = DK_MODEL_ACCESS->create_int_intrinsic_obj<uint64_t, VT_UI8>(end_addr);
                auto arg_mode = DK_MODEL_ACCESS->create_str_intrinsic_obj(mode);

                vector<DK_MOBJ_PTR> vec_args;
                vec_args.push_back(arg_start_addr);
                vec_args.push_back(arg_end_addr);
                vec_args.push_back(arg_mode);

                auto mem_access_result = DK_MODEL_ACCESS->call(mem_pobj, ttd, vec_args);
                if (mem_access_result != nullptr)
                {
                    auto results = DK_MODEL_ACCESS->iterate(mem_access_result);
                    for (auto& result : results)
                    {
                        ttd_mem_access mem_access;

                        mem_access.start_pos = get_pos(get<1>(result), "TimeStart");
                        mem_access.end_pos = get_pos(get<1>(result), "TimeEnd");
                        mem_access.access_type = BSTR2str(get_pvalue<BSTR, VT_BSTR>(get<1>(result), "AccessType"));
                        mem_access.ip_addr = get_pvalue<uint64_t, VT_UI8>(get<1>(result), "IP");
                        mem_access.addr = get_pvalue<uint64_t, VT_UI8>(get<1>(result), "Address");
                        mem_access.size = get_pvalue<uint64_t, VT_UI8>(get<1>(result), "Size");
                        mem_access.value = get_pvalue<uint64_t, VT_UI8>(get<1>(result), "Value");
                        mem_access.overwritten_value = get_pvalue<uint64_t, VT_UI8>(get<1>(result), "OverwrittenValue");
                        mem_access.thread_id = get_pvalue<uint64_t, VT_UI8>(get<1>(result), "ThreadId");
                        mem_access.event_type = get_pvalue<uint64_t, VT_UI8>(get<1>(result), "EventType");

                        mem_accesses.emplace_back(mem_access);
                    }
                }
            }
        }
    }

    return mem_accesses;
}

DK_MOBJ_PTR CModelAccess::get_current_session()
{
    return get_pobj_tree(m_debugger, "State.DebuggerVariables.cursession");
}

DK_MOBJ_PTR CModelAccess::get_current_process()
{
    return get_pobj_tree(m_debugger, "State.DebuggerVariables.curprocess");
}

DK_MOBJ_PTR CModelAccess::get_current_thread()
{
    return get_pobj_tree(m_debugger, "State.DebuggerVariables.curthread");
}

DK_MOBJ_PTR CModelAccess::get_current_stack()
{
    return get_pobj_tree(m_debugger, "State.DebuggerVariables.curstack");
}

DK_MOBJ_PTR CModelAccess::get_current_frame()
{
    return get_pobj_tree(m_debugger, "State.DebuggerVariables.curframe");
}

tuple<uint64_t, uint64_t> CModelAccess::get_current_pos()
{
    auto cur_pos = get_pobj_tree(m_debugger, "State.DebuggerVariables.curthread.TTD.Position");

    auto seq = get_pvalue<uint64_t, VT_UI8>(cur_pos, "Sequence");
    auto step = get_pvalue<uint64_t, VT_UI8>(cur_pos, "Steps");
    return make_tuple(seq, step);
}

string CModelAccess::dump_heap_memory(DK_MOBJ_PTR heap_memory)
{
    stringstream ss;

    auto function = BSTR2str(get_pvalue<BSTR, VT_BSTR>(heap_memory, "Function"));
    auto pos = get_pos(heap_memory, "Position");
    auto thread_id = get_pvalue<DWORD, VT_UI4>(heap_memory, "ThreadId");
    auto res_id = get_pvalue<uint64_t, VT_UI8>(heap_memory, "ResourceId");
    auto res_new_id = get_pvalue<uint64_t, VT_UI8>(heap_memory, "ResourceIdNew");
    auto size = get_pvalue<uint64_t, VT_UI8>(heap_memory, "Size");

    ss << hex << setw(10) << function
        << " (" << get<0>(pos) << " : " << get<1>(pos) << ") "
        << " tid: " << thread_id
        << " res_id: 0x" << res_id
        << " res_new_id: 0x" << res_new_id
        << " size: " << size;

    ss << endl;

    return ss.str();
}

vector<ttd_heap_memory> CModelAccess::get_heap_memory()
{
    vector<ttd_heap_memory> results;
    auto cur_session = DK_MODEL_ACCESS->get_current_session();

    if (cur_session != nullptr)
    {
        auto ttd = DK_MGET_POBJ(cur_session, "TTD");
        if (ttd != nullptr)
        {
            stringstream ss;
            auto resources = DK_MGET_POBJ(ttd, "Resources");
            auto heap_memory_pobj = DK_MGET_POBJ(resources, "HeapMemory");

            auto heap_memories = DK_MODEL_ACCESS->iterate(heap_memory_pobj);
            for (auto& heap_memory : heap_memories)
            {
                ttd_heap_memory heap_entry;

                heap_entry.function = BSTR2str(get_pvalue<BSTR, VT_BSTR>(get<1>(heap_memory), "Function"));
                heap_entry.pos = get_pos(get<1>(heap_memory), "Position");
                heap_entry.thread_id = get_pvalue<DWORD, VT_UI4>(get<1>(heap_memory), "ThreadId");
                heap_entry.res_id = get_pvalue<uint64_t, VT_UI8>(get<1>(heap_memory), "ResourceId");
                heap_entry.res_new_id = get_pvalue<uint64_t, VT_UI8>(get<1>(heap_memory), "ResourceIdNew");
                heap_entry.size = get_pvalue<uint64_t, VT_UI8>(get<1>(heap_memory), "Size");

                results.push_back(heap_entry);
            }
            EXT_F_STR_OUT(ss);
        }
    }

    return results;
}

string CModelAccess::dump_event(DK_MOBJ_PTR event)
{
    stringstream ss;

    auto type = BSTR2str(get_pvalue<BSTR, VT_BSTR>(event, "Type"));
    auto pos = DK_MGET_POS(event, "Position");

    ss << setw(20) << type
        << hex << " (" << get<0>(pos) << " : " << get<1>(pos) << ") ";

    auto module = get_pobj(event, "Module");

    if (module != nullptr)
    {
        ss << DK_DUMP(module)(module);
    }

    auto thread = get_pobj(event, "Thread");

    if (thread != nullptr)
    {
        ss << DK_DUMP(thread)(thread);
    }

    if (module == nullptr && thread == nullptr)
        ss << endl;

    return ss.str();
}

string CModelAccess::dump_handle(DK_MOBJ_PTR handle)
{
    stringstream ss;

    auto hid = get_pvalue<uint64_t, VT_UI8>(handle, "Handle");
    auto type = BSTR2str(get_pvalue<BSTR, VT_BSTR>(handle, "Type"));

    ss << " Handle: " << hid
        ;

    auto obj = get_pobj(handle, "Object");
    if (obj != nullptr)
    {
        Location loc;
        if (S_OK == obj->GetLocation(&loc))
        {
            ss << ", Object: 0x" << loc.Offset;
        }
    }

    ss << " , Type: " << type
        << endl;

    return ss.str();
}

string CModelAccess::dump_module(DK_MOBJ_PTR module)
{
    stringstream ss;

    auto module_info = get_module(module);

    ss << hex << " Base: " << get<0>(module_info)
        << " , Size: " << get<1>(module_info)
        << " , Name: " << get<2>(module_info)
        << endl;

    return ss.str();
}

vector<tuple<string, string, string, DK_MOBJ_PTR>> CModelAccess::ls(string model_path)
{
    vector<tuple<string, string, string, DK_MOBJ_PTR>> results;

    if (model_path == "Debugger")
        model_path = "State.DebuggerVariables.debuggerRootNamespace.Debugger";
    else if (model_path.size() > 9 && model_path.substr(0, 9) == "Debugger.")
        model_path = model_path.substr(9);

    DK_MOBJ_PTR mobj_main = DK_MODEL_ACCESS->get_pobj_tree(DK_MODEL_ACCESS->m_debugger, model_path);
    if (mobj_main != nullptr)
    {
        //string sepa_str(50, '=');
        //EXT_F_OUT("%s\n%s\n%s\n", sepa_str.c_str(), dump_mobj(mobj_main, false).c_str(), sepa_str.c_str());
        EXT_F_OUT("%s %s", model_path.c_str(), dump_mobj(mobj_main, false).c_str());
        for (auto& kv : enum_keyvalues(mobj_main))
        {
            stringstream ss;

            ss <<"!dk ls_model " << model_path << "." << get<0>(kv);
            results.emplace_back(make_tuple(get<0>(kv), get<1>(kv), ss.str(), get<2>(kv)));
        }

        auto iter_results = iterate(mobj_main);
        for (auto iter_result : iter_results)
        {
            auto kind = get_kind(get<1>(iter_result));

            stringstream ss;

            ss << "child #[ " << hex << get<0>(iter_result) << " ]";

            results.push_back(make_tuple(ss.str(), s_map_kind_name[kind], "", get<1>(iter_result)));
        }
    }

    return results;
}

ModelObjectKind CModelAccess::get_kind(DK_MOBJ_PTR& mobj)
{
    ModelObjectKind obj_kind;

    HRESULT hr = mobj->GetKind(&obj_kind);
    if (S_OK == hr)
    {
        return obj_kind;
    }

    return obj_kind;
}

template<class T, VARTYPE VT>
T CModelAccess::get_pvalue(DK_MOBJ_PTR& mobj, string key)
{
    auto pobj = get_pobj(mobj, key);

    if (pobj != nullptr)
    {
        VARIANT the_value;

        HRESULT hr = pobj->GetIntrinsicValueAs(VT, &the_value);
        if (S_OK == hr)
        {
            T result = reinterpret_cast<T>(the_value.pullVal);

            VariantClear(&the_value);
            return result;
        }

        VariantClear(&the_value);
    }

    return T(0);
}

template<class T, VARTYPE VT>
T CModelAccess::get_value(DK_MOBJ_PTR& mobj)
{
    VARIANT the_value;
    
    auto hr = mobj->GetIntrinsicValueAs(VT, &the_value);
    if (S_OK == hr)
    {
        T result = reinterpret_cast<T>(the_value.pullVal);

        VariantClear(&the_value);
        return result;
    }

    VariantClear(&the_value);
    return T(0);
}

DK_MOBJ_PTR CModelAccess::get_pobj(DK_MOBJ_PTR& mobj, string key)
{
    wstring wstr_key(key.begin(), key.end());
    DK_MOBJ_PTR pobj;

    HRESULT hr = mobj->GetKeyValue(wstr_key.c_str(), &pobj, nullptr);

    if (S_OK == hr)
    {
        //uint32_t* raw_ptr = (uint32_t*)pobj.Get();
        //if (*(raw_ptr + 3) != 1)
        //{
        //    EXT_F_OUT("Weird ref count: 0x%04x, %s\n", *(raw_ptr + 3), key.c_str());

        //    //DK_MOBJ_PTR pdobj;

        //    //hr = mobj->GetKey(wstr_key.c_str(), &pdobj, nullptr);
        //    //raw_ptr = (uint32_t*)pdobj.Get();

        //    //EXT_F_OUT("Weird ref count: 0x%04x, %s\n", *(raw_ptr + 3), key.c_str());
        //}

        return pobj;
    }

    return nullptr;
}

vector<string> str_split(const string& str, char delim) {
    stringstream ss(str);
    string item;

    vector<string> vec_sub_strs;
    while (getline(ss, item, delim)) {
        if (!item.empty()) {
            vec_sub_strs.push_back(item);
        }
    }
    return vec_sub_strs;
}

vector<wstring> wstr_split(const wstring& str, wchar_t delim) {
    wstringstream ss(str);
    wstring item;

    vector<wstring> vec_sub_strs;
    while (getline(ss, item, delim)) {
        if (!item.empty()) {
            vec_sub_strs.push_back(item);
        }
    }
    return vec_sub_strs;
}

DK_MOBJ_PTR CModelAccess::get_pobj_tree(DK_MOBJ_PTR& mobj, string key)
{
    wstring wstr_key(key.begin(), key.end());

    auto sub_strs = wstr_split(wstr_key, '.');
    DK_MOBJ_PTR ppobj = mobj;
    HRESULT hr = S_OK;

    for (auto& sub_str : sub_strs)
    {
        DK_MOBJ_PTR pobj;

        hr = ppobj->GetKeyValue(sub_str.c_str(), &pobj, nullptr);
        if (hr != S_OK || pobj == nullptr)
            return nullptr;  

        ppobj = pobj;
    }

    return ppobj;
}

DK_MOBJ_PTR CModelAccess::get_mobj_tree(string key)
{
    if (key == "Debugger")
    {
       key = "State.DebuggerVariables.debuggerRootNamespace.Debugger";

       return get_pobj_tree(m_debugger, key);
    }
    else if (key.size() > 9 && key.substr(0, 9) == "Debugger.")
    {
        key = key.substr(9);

        return get_pobj_tree(m_debugger, key);
    }
    return nullptr;
}

tuple<uint64_t, uint64_t> CModelAccess::get_pos(DK_MOBJ_PTR& mobj, string key)
{
    auto pos_pobj = get_pobj(mobj, key);
    if (pos_pobj != nullptr)
    {
        auto seq = get_pvalue<uint64_t, VT_UI8>(pos_pobj, "Sequence");
        auto step = get_pvalue<uint64_t, VT_UI8>(pos_pobj, "Steps");

        return make_tuple(seq, step);
    }

    return make_tuple<uint64_t, uint64_t>(0, 0);
}

tuple<uint64_t, uint64_t, string> CModelAccess::get_module(DK_MOBJ_PTR& mobj)
{
    auto module_name = BSTR2str(DK_MGET_PVAL<BSTR, VT_BSTR>(mobj, "Name"));
    auto module_size = DK_MGET_PVAL<uint64_t, VT_UI8>(mobj, "Size");
    auto module_base = DK_MGET_PVAL<uint64_t, VT_UI8>(mobj, "BaseAddress");

    return make_tuple(module_base, module_size, module_name);
}

DK_MOBJ_PTR CModelAccess::create_str_intrinsic_obj(string str)
{
    wstring wstr(str.begin(), str.end());

    BSTR bstrStr = SysAllocString(wstr.c_str());
    if (bstrStr == nullptr)
    {
        throw std::bad_alloc();
    }

    VARIANT vt_arg;
    vt_arg.vt = VT_BSTR;
    vt_arg.bstrVal = bstrStr;

    DK_MOBJ_PTR mobj_arg;

    if (S_OK == m_model_mgr->CreateIntrinsicObject(ObjectIntrinsic, &vt_arg, &mobj_arg))
    {
        VariantClear(&vt_arg);
        return mobj_arg;
    }

    VariantClear(&vt_arg);
    return nullptr;
}

template<class T, VARTYPE VT>
DK_MOBJ_PTR CModelAccess::create_int_intrinsic_obj(T val)
{
    VARIANT vt_arg;
    vt_arg.vt = VT;
    vt_arg.llVal = val;

    DK_MOBJ_PTR mobj_arg;

    if (S_OK == m_model_mgr->CreateIntrinsicObject(ObjectIntrinsic, &vt_arg, &mobj_arg))
    {
        VariantClear(&vt_arg);
        return mobj_arg;
    }

    VariantClear(&vt_arg);
    return nullptr;
}

DK_MOBJ_PTR CModelAccess::call(DK_MOBJ_PTR mobj, DK_MOBJ_PTR context, vector<DK_MOBJ_PTR> args)
{
    VARIANT variant_prop;
    auto hr = mobj->GetIntrinsicValue(&variant_prop);
    if (S_OK == hr)
    {
        IModelMethod* pMethod = static_cast<IModelMethod*>(variant_prop.punkVal);

        IModelObject** args_arr = new (nothrow) IModelObject * [args.size()];
        if (args_arr != nullptr)
        {
            for (auto i = 0; i < args.size(); i++)
                args_arr[i] = args[i].Get();

            DK_MOBJ_PTR call_result;
            hr = pMethod->Call(context.Get(), args.size(), args_arr, &call_result, nullptr);
            if (S_OK == hr)
            {
                VariantClear(&variant_prop);
                delete[] args_arr;
                return call_result;
            }

            delete[] args_arr;
        }
    }
    VariantClear(&variant_prop);
    return nullptr;
}

DK_MOBJ_PTR CModelAccess::assert_obj(DK_MOBJ_PTR& mobj)
{
    if (mobj == nullptr)
    {
        throw "model object is nullptr";
    }
    return DK_MOBJ_PTR();
}

vector<tuple<string, string, DK_MOBJ_PTR>> CModelAccess::enum_keyvalues(DK_MOBJ_PTR& mobj)
{
    vector<tuple<string, string, DK_MOBJ_PTR>> results;
    DK_COM_PTR<IKeyEnumerator> i_key_enumerator;
    HRESULT hr = mobj->EnumerateKeyValues(&i_key_enumerator);
    if (S_OK == hr)
    {
        BSTR key_name;
        DK_MOBJ_PTR key_mobj;
        while (E_BOUNDS != i_key_enumerator->GetNext(&key_name, &key_mobj, nullptr))
        {
            auto kind = get_kind(key_mobj);
            auto str_kind = s_map_kind_name[kind];

            results.push_back(make_tuple(BSTR2str(key_name), str_kind, key_mobj));
        }
    }

    return results;
}

vector<tuple<string, string, DK_MOBJ_PTR>> CModelAccess::enum_keys(DK_MOBJ_PTR& mobj)
{
    vector<tuple<string, string, DK_MOBJ_PTR>> results;
    DK_COM_PTR<IKeyEnumerator> i_key_enumerator;
    HRESULT hr = mobj->EnumerateKeys(&i_key_enumerator);
    if (S_OK == hr)
    {
        BSTR key_name;
        DK_MOBJ_PTR key_mobj;
        while (E_BOUNDS != i_key_enumerator->GetNext(&key_name, &key_mobj, nullptr))
        {
            auto kind = get_kind(key_mobj);
            auto str_kind = s_map_kind_name[kind];

            results.push_back(make_tuple(BSTR2str(key_name), str_kind, key_mobj));
        }
    }

    return results;
}

void CModelAccess::visit_root()
{
    DK_MOBJ_PTR root;
    HRESULT hr = m_model_mgr->GetRootNamespace(&root);
    if (SUCCEEDED(hr))
    {
        m_debugger = get_pobj(root, "Debugger");

        if (m_debugger != nullptr)
        {
            auto debugger_sessions = get_pobj(m_debugger, "Sessions");
            if (debugger_sessions != nullptr)
            {
                m_sessions = iterate(debugger_sessions);
                for (auto& session : m_sessions)
                {
                    auto attrs = get_pobj(get<1>(session), "Attributes");
                    if (attrs != nullptr)
                    {
                        auto target = get_pobj(attrs, "Target");

                        if (target != nullptr)
                        {
                            m_usermode = get_pvalue<bool, VT_BOOL>(target, "IsUserTarget");
                            m_kernelmode = get_pvalue<bool, VT_BOOL>(target, "IsKernelTarget");
                            m_ttdmode = get_pvalue<bool, VT_BOOL>(target, "IsTTDTarget");
                            m_dumpmode = get_pvalue<bool, VT_BOOL>(target, "IsDumpTarget");
                            m_livemode = get_pvalue<bool, VT_BOOL>(target, "IsLiveTarget");
                            m_nt = get_pvalue<bool, VT_BOOL>(target, "IsNTTarget");

                            if (m_ttdmode || m_dumpmode)
                            {
                                auto details = get_pobj(target, "Details");
                                if (details != nullptr)
                                {
                                    m_dump_filename = BSTR2str(get_pvalue<BSTR, VT_BSTR>(details, "DumpFileName"));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

DK_MOBJ_PTR CModelAccess::get_session(size_t session_id)
{
    for (auto& session : m_sessions)
    {
        if (get<0>(session) == session_id)
            return get<1>(session);
    }

    return nullptr;
}

DK_MOBJ_PTR CModelAccess::get_process(size_t session_id, size_t process_id)
{
    auto session = get_session(session_id);
    if (session != nullptr)
    {
        auto pobj_processes = get_pobj(session, "Processes");
        if (pobj_processes != nullptr)
        {
            return at(pobj_processes, process_id);
        }
    }

    return nullptr;
}

DK_MOBJ_PTR CModelAccess::get_thread(size_t session_id, size_t process_id, size_t thread_id)
{
    auto proc = get_process(session_id, process_id);
    if (proc != nullptr)
    {
        auto pobj_threads = get_pobj(proc, "Threads");
        if (pobj_threads != nullptr)
        {
            return at(pobj_threads, thread_id);
        }
    }

    return nullptr;
}

vector<tuple<uint64_t, DK_MOBJ_PTR>> CModelAccess::iterate(DK_MOBJ_PTR& mobj)
{
    vector<tuple<uint64_t, DK_MOBJ_PTR>> results;

    DK_COM_PTR<IIterableConcept> sp_iterable_concept;
    HRESULT hr = mobj->GetConcept(__uuidof(IIterableConcept), &sp_iterable_concept, nullptr);
    if (SUCCEEDED(hr))
    {
        ULONG64 demension = 0;

        hr = sp_iterable_concept->GetDefaultIndexDimensionality(mobj.Get(), &demension);
        if (S_OK == hr)
        {
            DK_COM_PTR<IModelIterator> sp_model_iterator;
            hr = sp_iterable_concept->GetIterator(mobj.Get(), &sp_model_iterator);

            if (demension != 0)
            {
                IModelObject** indexer_arr = new (nothrow) IModelObject * [demension];
                DK_MOBJ_PTR item;

                while (S_OK == sp_model_iterator->GetNext(&item, demension, indexer_arr, nullptr))
                {
                    VARIANT index_data = { 0, };

                    if (indexer_arr != nullptr)
                        hr = indexer_arr[0]->GetIntrinsicValueAs(VT_UI8, &index_data);

                    results.push_back(make_tuple(index_data.ullVal, item));
                    VariantClear(&index_data);
                }

                delete[] indexer_arr;
            }
            else
            {
                DK_MOBJ_PTR item;

                uint64_t index = 0;

                while (S_OK == sp_model_iterator->GetNext(&item, demension, nullptr, nullptr))
                {
                    results.push_back(make_tuple(index++, item));
                }
            }
        }
    }

    return results;
}

DK_MOBJ_PTR CModelAccess::path2mobj(string path)
{
    wstring str_model_path(path.begin(), path.end());

    DK_MOBJ_PTR mobj;

    auto hr = m_model_mgr->AcquireNamedModel(str_model_path.c_str(), &mobj);
    if (S_OK == hr)
    {
        return mobj;
    }

    return nullptr;
}

DK_MOBJ_PTR CModelAccess::at(DK_MOBJ_PTR& mobj, size_t index)
{
    DK_MOBJ_PTR result;

    DK_COM_PTR<IIndexableConcept> indexable_interface;
    HRESULT hr = mobj->GetConcept(__uuidof(IIndexableConcept), &indexable_interface, nullptr);
    if (SUCCEEDED(hr))
    {
        ULONG64 demension = 0;

        hr = indexable_interface->GetDimensionality(mobj.Get(), &demension);
        if (S_OK == hr)
        {
            IModelObject** index_arr = new (nothrow) IModelObject * [demension];
            if (index_arr != nullptr)
            {
                DK_MOBJ_PTR indexer;
                DK_MOBJ_PTR item;

                VARIANT vt_value;
                vt_value.vt = VT_UI8;
                vt_value.ullVal = index;

                if (SUCCEEDED(m_model_mgr->CreateIntrinsicObject(ObjectIntrinsic, &vt_value, &indexer)))
                {
                    index_arr[0] = indexer.Get();

                    hr = indexable_interface->GetAt(mobj.Get(), demension, index_arr, &item, nullptr);
                    if (S_OK == hr)
                    {
                        if (item != nullptr)
                            result = item;
                    }
                }

                delete[] index_arr;
                VariantClear(&vt_value);
            }
        }
    }

    return result;
}

CIModelObjectRefLogger::CIModelObjectRefLogger(IModelObject* raw_object, string tag)
    :m_raw_object(raw_object)
    , m_tag(tag)
{
   EXT_F_OUT("[+] 0x%0I64x %s %x\n", m_raw_object, m_tag.c_str(), ref_count());
}

CIModelObjectRefLogger::~CIModelObjectRefLogger()
{
    EXT_F_OUT("[-] 0x%0I64x %s %x\n", m_raw_object, m_tag.c_str(), ref_count());

    if (ref_count() > 0 && ref_count() <= 1000)
    {
        auto ref = m_raw_object->Release();
        EXT_F_OUT("[#] 0x%0I64x %s %d\n", m_raw_object, m_tag.c_str(), ref);
    }
}

