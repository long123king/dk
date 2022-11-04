#pragma once

#include <Windows.h>
#include <vcruntime_exception.h>
#include <DbgModel.h>
#include <memory>
#include <map>
#include <array>
#include <tuple>

#include <oaidl.h>
#include <wrl.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <wrl/module.h>

#include "CmdInterface.h"
#pragma warning( disable : 4244)

#define DK_COM_PTR              Microsoft::WRL::ComPtr

class CIModelObjectRefLogger
{
public:
    CIModelObjectRefLogger(IModelObject* raw_object, string tag);
    ~CIModelObjectRefLogger();

    uint32_t ref_count()
    {
        if (m_raw_object != nullptr)
        {
            uint32_t* raw_ptr = (uint32_t*)m_raw_object;
            return *(raw_ptr + 3);
        }
        return 0;
    }

private:
    IModelObject* m_raw_object;
    string m_tag;
};

//#define DK_DBG

#ifdef DK_DBG
#define LOG_DEFINE_MGR       vector<shared_ptr<CIModelObjectRefLogger>> g_log_mgr;
#define LOG_OBJ(mobj, tag)   mobj;g_log_mgr.emplace_back(make_shared<CIModelObjectRefLogger>(mobj.Get(), tag));
#define DK_DBG_MGET_POBJ(mobj, key)  CModelAccess::Instance()->get_pobj(mobj, key)
#define DK_MGET_POBJ(mobj, key)              LOG_OBJ(DK_DBG_MGET_POBJ(mobj, key), key); 
#define LOG_UNDEF_MGR        g_log_mgr.clear();
#else
#define LOG_DEFINE_MGR
#define LOG_OBJ(mobj, tag)
#define DK_MGET_POBJ            CModelAccess::Instance()->get_pobj
#define LOG_UNDEF_MGR
#endif

#define DK_MODEL_ACCESS         CModelAccess::Instance()
#define DK_MOBJ_PTR             DK_COM_PTR<IModelObject>

#define DK_DUMP(type)           CModelAccess::Instance()->dump_##type

#define DK_MGET_PVAL            CModelAccess::Instance()->get_pvalue
#define DK_MGET_VAL             CModelAccess::Instance()->get_value
#define DK_MGET_POS             CModelAccess::Instance()->get_pos
#define DK_MGET_KIND            CModelAccess::Instance()->get_kind
#define DK_MGET_MODL            CModelAccess::Instance()->get_module
#define DK_MGET_SESN            CModelAccess::Instance()->get_session
#define DK_MGET_PROC            CModelAccess::Instance()->get_process
#define DK_MGET_THRD            CModelAccess::Instance()->get_thread
#define DK_X_CMD                CModelAccess::Instance()->execute_cmd
#define DK_SEEK_TO              CModelAccess::Instance()->seek_to
#define DK_GET_CURSTACK         CModelAccess::Instance()->get_current_callstack
#define DK_DUMP_CURSTACK        CModelAccess::Instance()->dump_current_callstack
#define DK_GET_CURPOS           CModelAccess::Instance()->get_current_pos
#define DK_GET_CURTID           CModelAccess::Instance()->get_current_tid
#define DK_GET_HEAP             CModelAccess::Instance()->get_heap_memory
#define DK_GET_MEM_ACCESS       CModelAccess::Instance()->get_mem_access

DECLARE_CMD(ls_model)
DECLARE_CMD(ls_sessions)
DECLARE_CMD(ls_processes)
DECLARE_CMD(ls_threads)
DECLARE_CMD(ls_modules)
DECLARE_CMD(ls_handles)
DECLARE_CMD(ps_ttd)
DECLARE_CMD(session_ttd)
DECLARE_CMD(ttd_calls)
DECLARE_CMD(ttd_mem_access)
DECLARE_CMD(ttd_mem_use)
DECLARE_CMD(cur_context)
DECLARE_CMD(exec)
DECLARE_CMD(mobj)
DECLARE_CMD(mobj_at)
DECLARE_CMD(call)
DECLARE_CMD(ccall)

string BSTR2str(BSTR bstr);

typedef struct _ttd_heap_memory
{
    string                      function;
    tuple<uint64_t, uint64_t>   pos{ 0, 0 };
    uint32_t                    thread_id{ 0 };
    uint64_t                    res_id{ 0 };
    uint64_t                    res_new_id{ 0 };
    uint64_t                    size{ 0 };
}ttd_heap_memory;

typedef struct _ttd_mem_access
{
    string                      access_type;
    uint64_t                    ip_addr;
    uint64_t                    addr;
    uint64_t                    size;
    uint64_t                    value;
    uint64_t                    overwritten_value;
    uint64_t                    thread_id;
    uint64_t                    event_type;
    tuple<uint64_t, uint64_t>   start_pos{ 0, 0 };
    tuple<uint64_t, uint64_t>   end_pos{ 0, 0 };
}ttd_mem_access;

class CModelAccess
{
public:
    static CModelAccess* Instance()
    {
        static CModelAccess s_instance;
        return &s_instance;
    }

    CModelAccess();

    vector<tuple<string, string, string, DK_MOBJ_PTR>> ls(string model_path);

    vector<tuple<string, string, DK_MOBJ_PTR>> enum_keyvalues(DK_MOBJ_PTR& mobj);

    vector<tuple<string, string, DK_MOBJ_PTR>> enum_keys(DK_MOBJ_PTR& mobj);

    void visit_root();

    DK_MOBJ_PTR get_session(size_t session_id);

    DK_MOBJ_PTR get_process(size_t session_id, size_t process_id);

    DK_MOBJ_PTR get_thread(size_t session_id, size_t process_id, size_t thread_id);

public:
    vector<tuple<uint64_t, DK_MOBJ_PTR>> iterate(DK_MOBJ_PTR& mobj);

    DK_MOBJ_PTR path2mobj(string path);

    DK_MOBJ_PTR at(DK_MOBJ_PTR& mobj, size_t index);

    ModelObjectKind get_kind(DK_MOBJ_PTR& mobj);

    template<class T, VARTYPE VT>
    T get_pvalue(DK_MOBJ_PTR& mobj, string key);

    template<class T, VARTYPE VT>
    T get_value(DK_MOBJ_PTR& mobj);

    DK_MOBJ_PTR get_pobj(DK_MOBJ_PTR& mobj, string key);

    DK_MOBJ_PTR get_pobj_tree(DK_MOBJ_PTR& mobj, string key);

    DK_MOBJ_PTR get_mobj_tree(string key);

    tuple<uint64_t, uint64_t> get_pos(DK_MOBJ_PTR& mobj, string key);

    tuple<uint64_t, uint64_t, string> get_module(DK_MOBJ_PTR& mobj);

    DK_MOBJ_PTR create_str_intrinsic_obj(string str);

    template<class T, VARTYPE VT>
    DK_MOBJ_PTR create_int_intrinsic_obj(T val);

    DK_MOBJ_PTR call(DK_MOBJ_PTR mobj, DK_MOBJ_PTR context, vector<DK_MOBJ_PTR> args);

    DK_MOBJ_PTR assert_obj(DK_MOBJ_PTR& mobj);

    bool isUsermode() { 
        return m_usermode; 
    };
    bool isKernelmode() { 
        return m_kernelmode; 
    };
    bool isTTD() {
        return m_ttdmode;
    };
    bool isDump() {
        return m_dumpmode;
    };
    bool isLive() {
        return m_livemode;
    };
    bool isNT() {
        return m_nt;
    };
    string dumpFilename(){
        return m_dump_filename;
    }

    string dump_frame(DK_MOBJ_PTR frame);

    string dump_thread(DK_MOBJ_PTR thread);

    string dump_process(DK_MOBJ_PTR process);
     
    string dump_stack(DK_MOBJ_PTR stack);

    string dump_current_callstack();

    string dump_module(DK_MOBJ_PTR module);

    string dump_session(DK_MOBJ_PTR session);

    string dump_handle(DK_MOBJ_PTR handle);

    string dump_event(DK_MOBJ_PTR event);

    string dump_call_result(DK_MOBJ_PTR call_result);

    string dump_heap_memory(DK_MOBJ_PTR heap_memory);

    vector<ttd_heap_memory> get_heap_memory();

    string dump_mem_access_result(DK_MOBJ_PTR mem_access_result);

    string dump_mem_use_result(DK_MOBJ_PTR mem_use_result);

    vector<array<uint64_t, 5>> get_current_callstack();

    void seek_to(uint64_t seq, uint64_t step);

    string dump_mobj(DK_MOBJ_PTR mobj, bool b_show_children = true, string mobj_path = "");

    vector<string> execute_cmd(string command);

    uint64_t get_current_tid();

    vector<ttd_mem_access> get_mem_access(uint64_t start_addr, uint64_t end_addr, string mode);

    DK_MOBJ_PTR get_current_session();
    DK_MOBJ_PTR get_current_process();
    DK_MOBJ_PTR get_current_thread();
    DK_MOBJ_PTR get_current_stack();
    DK_MOBJ_PTR get_current_frame();
    tuple<uint64_t, uint64_t> get_current_pos();

private:
    IHostDataModelAccess*   m_model_access{ nullptr };
    IDataModelManager*      m_model_mgr{ nullptr };
    IDebugHost*             m_debug_host{ nullptr };

    static map<ModelObjectKind, string> s_map_kind_name;
    static map<VARTYPE, string> s_map_vt_name;

    bool m_usermode = { false };
    bool m_kernelmode = { false };
    bool m_ttdmode = { false };
    bool m_dumpmode = { false };
    bool m_livemode = { false };
    bool m_nt = { false };

    string m_dump_filename;

public:
    DK_MOBJ_PTR m_debugger;

    DK_MOBJ_PTR m_cur_proc;
    DK_MOBJ_PTR m_cur_thread;
    DK_MOBJ_PTR m_cur_session;

    vector<tuple<uint64_t, DK_MOBJ_PTR>> m_sessions;
};
