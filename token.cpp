#include "token.h"
#include "process.h"
#include "CmdExt.h"
#include "BitFieldAnalyzer.h"

#include <iomanip>

const std::map<std::string, std::string> s_integrity_level_texts{ {
    { "S-1-16-0",         "Untrusted(0)" },
    { "S-1-16-4096",      "Low(1)" },
    { "S-1-16-8192",      "Medium(2)" },
    { "S-1-16-12288",     "High(3)" },
    { "S-1-16-16384",     "System(4)" },
    { "S-1-16-20480",     "Protected(5)" },
    } };

const std::map<std::string, std::string> s_trust_label_texts{ {
    { "S-1-19-512-4096",          "Trust Label Lite(PPL) PsProtectedSignerWindows(5)" },
    { "S-1-19-1024-4096",         "Trust Label Protected(PP) PsProtectedSignerWindows(5)" },
    { "S-1-19-512-8192",          "Trust Label Lite(PPL) PsProtectedSignerTcb(6)" },
    { "S-1-19-1024-8192",         "Trust Label Protected(PP) PsProtectedSignerTcb(6)" }
    } };


const std::map<std::string, std::string> s_wellknown_sids{ {
    { "S-1-0",				"Null"},
    { "S-1-1-0",			"Everyone" },
    { "S-1-2-0",			"Local" },
    { "S-1-2-1",			"Console Logon" },
    { "S-1-3",				"Creator Authority"},
    { "S-1-3-0",			"Creator Owner"},
    { "S-1-3-1",			"Creator Group"},
    { "S-1-3-4",			"Owner Rights"},
    { "S-1-5-2",			"Network"},
    { "S-1-5-3",			"BATCH"},
    { "S-1-5-33",			"WRITE RESTRICTED CODE"},
    { "S-1-5-4",			"Interactive"},
    { "S-1-5-6",			"Service"},
    { "S-1-5-7",			"Anonymous"},
    { "S-1-5-9",			"Enterprise Domain Controllers"},
    { "S-1-5-10",			"Principal Self"},
    { "S-1-5-11",			"Authenticated Users"},
    { "S-1-5-12",			"Restricted Code"},
    { "S-1-5-13",			"Terminal Server Users"},
    { "S-1-5-14",			"Remote Interactive Logon"},
    { "S-1-5-15",			"This Organization"},
    { "S-1-5-17",			"IUSR"},
    { "S-1-5-18",           "Local System" },
    { "S-1-5-19",			"NT Authority/Local Service"},
    { "S-1-5-20",			"NT Authority/Network Service"},
    { "S-1-5-32-544",		"BUILTIN/Administrators" },
    { "S-1-5-32-545",		"BUILTIN/Users" },
    { "S-1-5-32-546",		"BUILTIN/Guests" },
    { "S-1-5-32-549",		"BUILTIN/SERVER OPERATORS" },
    { "S-1-5-32-550",		"BUILTIN/PRINTER OPERATORS" },
    { "S-1-5-32-551",		"BUILTIN/BACKUP OPERATORS" },
    { "S-1-5-32-555",		"BUILTIN/Remote Desktop Users" },
    { "S-1-5-32-559",		"BUILTIN/Performance Log Users"},
    { "S-1-5-32-558",		"BUILTIN/Performance Monitor Users"},
    { "S-1-5-32-578",		"BUILTIN/Hyper-V Administrators" },
    { "S-1-5-80-0",			"All Services"},
    { "S-1-5-113",			"Local Account"},
    { "S-1-5-114",			"Local Account&Member of Admins Group"},
    { "S-1-5-64-10",		"NTLM Authentication"},
    { "S-1-15-2-1",			"ALL APP PACKAGES"},
} };

std::string
getGroupsAttrText(
	__in uint32_t attr,
	__in bool     pureText = false
)
{
	static CBitFieldAnalyzer s_GroupsAttrAnalyzer{ {
		{ 0x00000004, "enabled" },
		{ 0x00000002, "default" },
		{ 0x00000020, "integrity" },
		{ 0x00000040, "integrity-enabled" },
		{ 0xC0000000, "logon-id" },
		{ 0x00000001, "mandatory" },
		{ 0x00000008, "owner" },
		{ 0x00000010, "deny-only" },
		{ 0x20000000, "resource" }
		} };

	return s_GroupsAttrAnalyzer.GetText(attr, pureText);
}

std::string
getAceTypeStr(
	__in const size_t ace_type
)
{
	static std::map<size_t, const char*> s_ace_type_map{
		{ ACCESS_ALLOWED_ACE_TYPE				 , "[Allow]" },
		{ ACCESS_DENIED_ACE_TYPE			     , "[Deny ]" },
		{ SYSTEM_AUDIT_ACE_TYPE					 , "[Audit]" },
		{ SYSTEM_ALARM_ACE_TYPE					 , "[Alarm]" },
		{ ACCESS_ALLOWED_COMPOUND_ACE_TYPE        , "[Allow_Compound]" },
		{ ACCESS_ALLOWED_OBJECT_ACE_TYPE          , "[Allow_Object]" },
		{ ACCESS_DENIED_OBJECT_ACE_TYPE           , "[Deny_Object]" },
		{ SYSTEM_AUDIT_OBJECT_ACE_TYPE            , "[Audit_Object]" },
		{ SYSTEM_ALARM_OBJECT_ACE_TYPE            , "[Alarm_Object]" },
		{ ACCESS_ALLOWED_CALLBACK_ACE_TYPE        , "[Allow_Callback]" },
		{ ACCESS_DENIED_CALLBACK_ACE_TYPE         , "[Deny_Callback]" },
		{ ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE , "[Allow_Callback_Object]" },
		{ ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE  , "[Deny_Callback_Object]" },
		{ SYSTEM_AUDIT_CALLBACK_ACE_TYPE          , "[Audit_Callback]" },
		{ SYSTEM_ALARM_CALLBACK_ACE_TYPE          , "[Alarm_Callback]" },
		{ SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE   , "[Audit_Callback_Object]" },
		{ SYSTEM_ALARM_CALLBACK_OBJECT_ACE_TYPE   , "[Alarm_Callback_Object]" },
		{ SYSTEM_MANDATORY_LABEL_ACE_TYPE         , "[Madatory_Label]" },
		{ SYSTEM_RESOURCE_ATTRIBUTE_ACE_TYPE      , "[Resource_Attribute]" },
		{ SYSTEM_SCOPED_POLICY_ID_ACE_TYPE        , "[Scoped_Policy_Id]" },
		{ SYSTEM_PROCESS_TRUST_LABEL_ACE_TYPE     , "[Process_Trust_Label]" },
	};

	auto it = s_ace_type_map.find(ace_type);
	if (it != s_ace_type_map.end())
		return it->second;

	return "[     ]";
}


std::string
getTokenSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {
		{ TOKEN_ASSIGN_PRIMARY, "TOKEN_ASSIGN_PRIMARY"},  // #define TOKEN_ASSIGN_PRIMARY    (0x0001)
		{ TOKEN_DUPLICATE, "TOKEN_DUPLICATE" },  // #define TOKEN_DUPLICATE         (0x0002)
		{ TOKEN_IMPERSONATE, "TOKEN_IMPERSONATE" },  // #define TOKEN_IMPERSONATE       (0x0004)
		{ TOKEN_QUERY, "TOKEN_QUERY" },  // #define TOKEN_QUERY             (0x0008)
		{ TOKEN_QUERY_SOURCE, "TOKEN_QUERY_SOURCE" },  // #define TOKEN_QUERY_SOURCE      (0x0010)
		{ TOKEN_ADJUST_PRIVILEGES, "TOKEN_ADJUST_PRIVILEGES" },  // #define TOKEN_ADJUST_PRIVILEGES (0x0020)
		{ TOKEN_ADJUST_GROUPS, "TOKEN_ADJUST_GROUPS" },  // #define TOKEN_ADJUST_GROUPS     (0x0040)
		{ TOKEN_ADJUST_DEFAULT, "TOKEN_ADJUST_DEFAULT" },  // #define TOKEN_ADJUST_DEFAULT    (0x0080)
		{ TOKEN_ADJUST_SESSIONID, "TOKEN_ADJUST_SESSIONID" },  // #define TOKEN_ADJUST_SESSIONID  (0x0100)
	} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getProcessSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {//
		{ PROCESS_TERMINATE, "PROCESS_TERMINATE" },  // #define PROCESS_TERMINATE                  (0x0001)
		{ PROCESS_CREATE_THREAD, "PROCESS_CREATE_THREAD" },  // #define PROCESS_CREATE_THREAD              (0x0002)
		{ PROCESS_SET_SESSIONID, "PROCESS_SET_SESSIONID" },  // #define PROCESS_SET_SESSIONID              (0x0004)
		{ PROCESS_VM_OPERATION, "PROCESS_VM_OPERATION" },  // #define PROCESS_VM_OPERATION               (0x0008)
		{ PROCESS_VM_READ, "PROCESS_VM_READ" },  // #define PROCESS_VM_READ                    (0x0010)
		{ PROCESS_VM_WRITE, "PROCESS_VM_WRITE" },  // #define PROCESS_VM_WRITE                   (0x0020)
		{ PROCESS_DUP_HANDLE, "PROCESS_DUP_HANDLE" },  // #define PROCESS_DUP_HANDLE                 (0x0040)
		{ PROCESS_CREATE_PROCESS, "PROCESS_CREATE_PROCESS" },  // #define PROCESS_CREATE_PROCESS             (0x0080)
		{ PROCESS_SET_QUOTA, "PROCESS_SET_QUOTA" },  // #define PROCESS_SET_QUOTA                  (0x0100)
		{ PROCESS_SET_INFORMATION, "PROCESS_SET_INFORMATION" },  // #define PROCESS_SET_INFORMATION            (0x0200)
		{ PROCESS_QUERY_INFORMATION, "PROCESS_QUERY_INFORMATION" },  // #define PROCESS_QUERY_INFORMATION          (0x0400)
		{ PROCESS_SUSPEND_RESUME, "PROCESS_SUSPEND_RESUME" },  // #define PROCESS_SUSPEND_RESUME             (0x0800)
		{ PROCESS_QUERY_LIMITED_INFORMATION, "PROCESS_QUERY_LIMITED_INFORMATION" },  // #define PROCESS_QUERY_LIMITED_INFORMATION  (0x1000)
		{ PROCESS_SET_LIMITED_INFORMATION, "PROCESS_SET_LIMITED_INFORMATION" },  // #define PROCESS_SET_LIMITED_INFORMATION    (0x2000)
		} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getThreadSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {//
	   { THREAD_TERMINATE, "THREAD_TERMINATE" },  // #define THREAD_TERMINATE                 (0x0001)
	   { THREAD_SUSPEND_RESUME, "THREAD_SUSPEND_RESUME" },  // #define THREAD_SUSPEND_RESUME            (0x0002)
	   { THREAD_GET_CONTEXT, "THREAD_GET_CONTEXT" },  // #define THREAD_GET_CONTEXT               (0x0008)
	   { THREAD_SET_CONTEXT, "THREAD_SET_CONTEXT" },  // #define THREAD_SET_CONTEXT               (0x0010)
	   { THREAD_QUERY_INFORMATION, "THREAD_QUERY_INFORMATION" },  // #define THREAD_QUERY_INFORMATION         (0x0040)
	   { THREAD_SET_INFORMATION, "THREAD_SET_INFORMATION" },  // #define THREAD_SET_INFORMATION           (0x0020)
	   { THREAD_SET_THREAD_TOKEN, "THREAD_SET_THREAD_TOKEN" },  // #define THREAD_SET_THREAD_TOKEN          (0x0080)
	   { THREAD_IMPERSONATE, "THREAD_IMPERSONATE" },  // #define THREAD_IMPERSONATE               (0x0100)
	   { THREAD_DIRECT_IMPERSONATION, "THREAD_DIRECT_IMPERSONATION" },  // #define THREAD_DIRECT_IMPERSONATION      (0x0200)
	   { THREAD_SET_LIMITED_INFORMATION, "THREAD_SET_LIMITED_INFORMATION" },  // #define THREAD_SET_LIMITED_INFORMATION   (0x0400)
	   { THREAD_QUERY_LIMITED_INFORMATION, "THREAD_QUERY_LIMITED_INFORMATION" },  // #define THREAD_QUERY_LIMITED_INFORMATION (0x0800)
	   { THREAD_RESUME, "THREAD_RESUME" },  // #define THREAD_RESUME                    (0x1000)
	   } };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getJobSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ { //
		{ JOB_OBJECT_ASSIGN_PROCESS, "JOB_OBJECT_ASSIGN_PROCESS" },  // #define JOB_OBJECT_ASSIGN_PROCESS           (0x0001)
		{ JOB_OBJECT_SET_ATTRIBUTES, "JOB_OBJECT_SET_ATTRIBUTES" },  // #define JOB_OBJECT_SET_ATTRIBUTES           (0x0002)
		{ JOB_OBJECT_QUERY, "JOB_OBJECT_QUERY" },  // #define JOB_OBJECT_QUERY                    (0x0004)
		{ JOB_OBJECT_TERMINATE, "JOB_OBJECT_TERMINATE" },  // #define JOB_OBJECT_TERMINATE                (0x0008)
		{ JOB_OBJECT_SET_SECURITY_ATTRIBUTES, "JOB_OBJECT_SET_SECURITY_ATTRIBUTES" },  // #define JOB_OBJECT_SET_SECURITY_ATTRIBUTES  (0x0010)
		} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getEventSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {				   //
		{ EVENT_MODIFY_STATE, "EVENT_MODIFY_STATE" },  // #define EVENT_MODIFY_STATE      0x0002
	} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getMutantSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ { //
		{ MUTANT_QUERY_STATE, "MUTANT_QUERY_STATE" },  // #define MUTANT_QUERY_STATE      0x0001
	} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getSemaphoreSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ { //
		{ SEMAPHORE_MODIFY_STATE, "SEMAPHORE_MODIFY_STATE" },  // #define SEMAPHORE_MODIFY_STATE      0x0002
	} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getTimerSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {		   //
		{ TIMER_QUERY_STATE, "TIMER_QUERY_STATE" },  // #define TIMER_QUERY_STATE       0x0001
		{ TIMER_MODIFY_STATE, "TIMER_MODIFY_STATE" },  // #define TIMER_MODIFY_STATE      0x0002
	} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getSectionSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {   //
		{ SECTION_QUERY, "SECTION_QUERY" },  // #define SECTION_QUERY                0x0001
		{ SECTION_MAP_WRITE, "SECTION_MAP_WRITE" },  // #define SECTION_MAP_WRITE            0x0002
		{ SECTION_MAP_READ, "SECTION_MAP_READ" },  // #define SECTION_MAP_READ             0x0004
		{ SECTION_MAP_EXECUTE, "SECTION_MAP_EXECUTE" },  // #define SECTION_MAP_EXECUTE          0x0008
		{ SECTION_EXTEND_SIZE, "SECTION_EXTEND_SIZE" },  // #define SECTION_EXTEND_SIZE          0x0010
		{ SECTION_MAP_EXECUTE_EXPLICIT, "SECTION_MAP_EXECUTE_EXPLICIT" },  // #define SECTION_MAP_EXECUTE_EXPLICIT 0x0020
	} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getFileSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {					   //
			// // File
				{ FILE_READ_DATA, "FILE_READ_DATA" },  // #define FILE_READ_DATA            ( 0x0001 )    // file & pipe
				{ FILE_WRITE_DATA, "FILE_WRITE_DATA" },  // #define FILE_WRITE_DATA           ( 0x0002 )    // file & pipe
				{ FILE_APPEND_DATA, "FILE_APPEND_DATA" },  // #define FILE_APPEND_DATA          ( 0x0004 )    // file
				{ FILE_READ_EA, "FILE_READ_EA" },  // #define FILE_READ_EA              ( 0x0008 )    // file & directory
				{ FILE_WRITE_EA, "FILE_WRITE_EA" },  // #define FILE_WRITE_EA             ( 0x0010 )    // file & directory
				{ FILE_EXECUTE, "FILE_EXECUTE" },  // #define FILE_EXECUTE              ( 0x0020 )    // file
				{ FILE_READ_ATTRIBUTES, "FILE_READ_ATTRIBUTES" },  // #define FILE_READ_ATTRIBUTES      ( 0x0080 )    // all
				{ FILE_WRITE_ATTRIBUTES, "FILE_WRITE_ATTRIBUTES" },  // #define FILE_WRITE_ATTRIBUTES     ( 0x0100 )    // all
			} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getDirectorySpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {		 //
			// // Directory
				{ FILE_LIST_DIRECTORY, "FILE_LIST_DIRECTORY" },  // #define FILE_LIST_DIRECTORY       ( 0x0001 )    // directory
				{ FILE_ADD_FILE, "FILE_ADD_FILE" },  // #define FILE_ADD_FILE             ( 0x0002 )    // directory
				{ FILE_ADD_SUBDIRECTORY, "FILE_ADD_SUBDIRECTORY" },  // #define FILE_ADD_SUBDIRECTORY     ( 0x0004 )    // directory
				{ FILE_READ_EA, "FILE_READ_EA" },  // #define FILE_READ_EA              ( 0x0008 )    // file & directory
				{ FILE_WRITE_EA, "FILE_WRITE_EA" },  // #define FILE_WRITE_EA             ( 0x0010 )    // file & directory
				{ FILE_TRAVERSE, "FILE_TRAVERSE" },  // #define FILE_TRAVERSE             ( 0x0020 )    // directory
				{ FILE_DELETE_CHILD, "FILE_DELETE_CHILD" },  // #define FILE_DELETE_CHILD         ( 0x0040 )    // directory
				{ FILE_READ_ATTRIBUTES, "FILE_READ_ATTRIBUTES" },  // #define FILE_READ_ATTRIBUTES      ( 0x0080 )    // all
				{ FILE_WRITE_ATTRIBUTES, "FILE_WRITE_ATTRIBUTES" },  // #define FILE_WRITE_ATTRIBUTES     ( 0x0100 )    // all
			} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getPipeSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {		 //
			// // Pipe
				{ FILE_READ_DATA, "FILE_READ_DATA" },  // #define FILE_READ_DATA            ( 0x0001 )    // file & pipe
				{ FILE_WRITE_DATA, "FILE_WRITE_DATA" },  // #define FILE_WRITE_DATA           ( 0x0002 )    // file & pipe
				{ FILE_CREATE_PIPE_INSTANCE, "FILE_CREATE_PIPE_INSTANCE" },  // #define FILE_CREATE_PIPE_INSTANCE ( 0x0004 )    // named pipe
				{ FILE_READ_ATTRIBUTES, "FILE_READ_ATTRIBUTES" },  // #define FILE_READ_ATTRIBUTES      ( 0x0080 )    // all
				{ FILE_WRITE_ATTRIBUTES, "FILE_WRITE_ATTRIBUTES" },  // #define FILE_WRITE_ATTRIBUTES     ( 0x0100 )    // all
			} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getIoCSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {		 //
		{ IO_COMPLETION_MODIFY_STATE, "IO_COMPLETION_MODIFY_STATE" },  // #define IO_COMPLETION_MODIFY_STATE  0x0002
	} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getKeySpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {					   //
		{ KEY_QUERY_VALUE, "KEY_QUERY_VALUE" },  // #define KEY_QUERY_VALUE         (0x0001)
		{ KEY_SET_VALUE, "KEY_SET_VALUE" },  // #define KEY_SET_VALUE           (0x0002)
		{ KEY_CREATE_SUB_KEY, "KEY_CREATE_SUB_KEY" },  // #define KEY_CREATE_SUB_KEY      (0x0004)
		{ KEY_ENUMERATE_SUB_KEYS, "KEY_ENUMERATE_SUB_KEYS" },  // #define KEY_ENUMERATE_SUB_KEYS  (0x0008)
		{ KEY_NOTIFY, "KEY_NOTIFY" },  // #define KEY_NOTIFY              (0x0010)
		{ KEY_CREATE_LINK, "KEY_CREATE_LINK" },  // #define KEY_CREATE_LINK         (0x0020)
		{ KEY_WOW64_32KEY, "KEY_WOW64_32KEY" },  // #define KEY_WOW64_32KEY         (0x0200)
		{ KEY_WOW64_64KEY, "KEY_WOW64_64KEY" },  // #define KEY_WOW64_64KEY         (0x0100)
	} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getTxMSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ { //
		{ TRANSACTIONMANAGER_QUERY_INFORMATION, "TRANSACTIONMANAGER_QUERY_INFORMATION" },  // #define TRANSACTIONMANAGER_QUERY_INFORMATION     ( 0x0001 )
		{ TRANSACTIONMANAGER_SET_INFORMATION, "TRANSACTIONMANAGER_SET_INFORMATION" },  // #define TRANSACTIONMANAGER_SET_INFORMATION       ( 0x0002 )
		{ TRANSACTIONMANAGER_RECOVER, "TRANSACTIONMANAGER_RECOVER" },  // #define TRANSACTIONMANAGER_RECOVER               ( 0x0004 )
		{ TRANSACTIONMANAGER_RENAME, "TRANSACTIONMANAGER_RENAME" },  // #define TRANSACTIONMANAGER_RENAME                ( 0x0008 )
		{ TRANSACTIONMANAGER_CREATE_RM, "TRANSACTIONMANAGER_CREATE_RM" },  // #define TRANSACTIONMANAGER_CREATE_RM             ( 0x0010 )
		{ TRANSACTIONMANAGER_BIND_TRANSACTION, "TRANSACTIONMANAGER_BIND_TRANSACTION" },  // #define TRANSACTIONMANAGER_BIND_TRANSACTION      ( 0x0020 )
	} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getTxSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {										 //
		{ TRANSACTION_QUERY_INFORMATION, "TRANSACTION_QUERY_INFORMATION" },  // #define TRANSACTION_QUERY_INFORMATION     ( 0x0001 )
		{ TRANSACTION_SET_INFORMATION, "TRANSACTION_SET_INFORMATION" },  // #define TRANSACTION_SET_INFORMATION       ( 0x0002 )
		{ TRANSACTION_ENLIST, "TRANSACTION_ENLIST" },  // #define TRANSACTION_ENLIST                ( 0x0004 )
		{ TRANSACTION_COMMIT, "TRANSACTION_COMMIT" },  // #define TRANSACTION_COMMIT                ( 0x0008 )
		{ TRANSACTION_ROLLBACK, "TRANSACTION_ROLLBACK" },  // #define TRANSACTION_ROLLBACK              ( 0x0010 )
		{ TRANSACTION_PROPAGATE, "TRANSACTION_PROPAGATE" },  // #define TRANSACTION_PROPAGATE             ( 0x0020 )
		{ TRANSACTION_RIGHT_RESERVED1, "TRANSACTION_RIGHT_RESERVED1" },  // #define TRANSACTION_RIGHT_RESERVED1       ( 0x0040 )
	} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getRMSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {							 //
		{ RESOURCEMANAGER_QUERY_INFORMATION, "RESOURCEMANAGER_QUERY_INFORMATION" },  // #define RESOURCEMANAGER_QUERY_INFORMATION     ( 0x0001 )
		{ RESOURCEMANAGER_SET_INFORMATION, "RESOURCEMANAGER_SET_INFORMATION" },  // #define RESOURCEMANAGER_SET_INFORMATION       ( 0x0002 )
		{ RESOURCEMANAGER_RECOVER, "RESOURCEMANAGER_RECOVER" },  // #define RESOURCEMANAGER_RECOVER               ( 0x0004 )
		{ RESOURCEMANAGER_ENLIST, "RESOURCEMANAGER_ENLIST" },  // #define RESOURCEMANAGER_ENLIST                ( 0x0008 )
		{ RESOURCEMANAGER_GET_NOTIFICATION, "RESOURCEMANAGER_GET_NOTIFICATION" },  // #define RESOURCEMANAGER_GET_NOTIFICATION      ( 0x0010 )
		{ RESOURCEMANAGER_REGISTER_PROTOCOL, "RESOURCEMANAGER_REGISTER_PROTOCOL" },  // #define RESOURCEMANAGER_REGISTER_PROTOCOL     ( 0x0020 )
		{ RESOURCEMANAGER_COMPLETE_PROPAGATION, "RESOURCEMANAGER_COMPLETE_PROPAGATION" },  // #define RESOURCEMANAGER_COMPLETE_PROPAGATION  ( 0x0040 )
	} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getEnlistSpecificAccess(
	__in size_t access,
	__in bool     pureText = false)
{
	static CBitFieldAnalyzer s_SpecificAccessAnalyzer{ {											   //
		{ ENLISTMENT_QUERY_INFORMATION, "ENLISTMENT_QUERY_INFORMATION" },  // #define ENLISTMENT_QUERY_INFORMATION     ( 0x0001 )
		{ ENLISTMENT_SET_INFORMATION, "ENLISTMENT_SET_INFORMATION" },  // #define ENLISTMENT_SET_INFORMATION       ( 0x0002 )
		{ ENLISTMENT_RECOVER, "ENLISTMENT_RECOVER" },  // #define ENLISTMENT_RECOVER               ( 0x0004 )
		{ ENLISTMENT_SUBORDINATE_RIGHTS, "ENLISTMENT_SUBORDINATE_RIGHTS" },  // #define ENLISTMENT_SUBORDINATE_RIGHTS    ( 0x0008 )
		{ ENLISTMENT_SUPERIOR_RIGHTS, "ENLISTMENT_SUPERIOR_RIGHTS" },  // #define ENLISTMENT_SUPERIOR_RIGHTS       ( 0x0010 )
	} };

	return s_SpecificAccessAnalyzer.GetText((uint32_t)access, pureText);
}

std::string
getAceMaskStr(
	__in const size_t ace_mask,
	__in std::string type_name,
	__in bool     pureText
)
{
	static CBitFieldAnalyzer s_AceMaskAnalyzer{ {
		{DELETE                  , "DELETE"},                           //(0x00010000L)
		{READ_CONTROL            , "READ_CONTROL"},                     //(0x00020000L)
		{WRITE_DAC               , "WRITE_DAC" },                       //(0x00040000L)
		{WRITE_OWNER             , "WRITE_OWNER" },                     //(0x00080000L)
		{SYNCHRONIZE             , "SYNCHRONIZE" },                     //(0x00100000L)
		{ACCESS_SYSTEM_SECURITY  , "ACCESS_SYSTEM_SECURITY" },          //(0x01000000L)
		{MAXIMUM_ALLOWED         , "MAXIMUM_ALLOWED" },                 //(0x02000000L)
		{GENERIC_READ            , "GENERIC_READ" },                    //(0x80000000L)
		{GENERIC_WRITE           , "GENERIC_WRITE" },                   //(0x40000000L)
		{GENERIC_EXECUTE         , "GENERIC_EXECUTE" },                 //(0x20000000L)
		{GENERIC_ALL             , "GENERIC_ALL" },                     //(0x10000000L)
		} };

	std::string generic_mask_str = s_AceMaskAnalyzer.GetText(ace_mask & 0xFFFF0000, pureText);

	auto specific_mask = ace_mask & 0xffff;

	generic_mask_str += " ";
	generic_mask_str += type_name;
	generic_mask_str += ": ";

	if (type_name == "Token")
		generic_mask_str += getTokenSpecificAccess(specific_mask, pureText);
	else if (type_name == "Process")
		generic_mask_str += getProcessSpecificAccess(specific_mask, pureText);
	else if (type_name == "Thread")
		generic_mask_str += getThreadSpecificAccess(specific_mask, pureText);
	else if (type_name == "Directory")
		generic_mask_str += getDirectorySpecificAccess(specific_mask, pureText);
	else if (type_name == "Section")
		generic_mask_str += getSectionSpecificAccess(specific_mask, pureText);
	else if (type_name == "Mutant")
		generic_mask_str += getMutantSpecificAccess(specific_mask, pureText);
	else if (type_name == "Semaphore")
		generic_mask_str += getSemaphoreSpecificAccess(specific_mask, pureText);
	else if (type_name == "Event")
		generic_mask_str += getEventSpecificAccess(specific_mask, pureText);
	else if (type_name == "TmTx")
		generic_mask_str += getTxSpecificAccess(specific_mask, pureText);
	else if (type_name == "TmTm")
		generic_mask_str += getTxMSpecificAccess(specific_mask, pureText);
	else if (type_name == "TmRm")
		generic_mask_str += getRMSpecificAccess(specific_mask, pureText);
	else if (type_name == "Timer")
		generic_mask_str += getTimerSpecificAccess(specific_mask, pureText);
	else if (type_name == "Job")
		generic_mask_str += getJobSpecificAccess(specific_mask, pureText);
	else if (type_name == "Key")
		generic_mask_str += getKeySpecificAccess(specific_mask, pureText);
	else if (type_name == "TmEn")
		generic_mask_str += getEnlistSpecificAccess(specific_mask, pureText);
	else if (type_name == "IoCompletion")
		generic_mask_str += getIoCSpecificAccess(specific_mask, pureText);
	else// if (type_name == "File")
		generic_mask_str += getFileSpecificAccess(specific_mask, pureText);

	return generic_mask_str;
}


DEFINE_CMD(sid)
{
	if (args.size() < 2)
	{
		EXT_F_ERR("Usage: !dk sid <sid>\n");
	}

	size_t sid_addr = EXT_F_IntArg(args, 1, 0);
	std::tuple<std::string, std::string> tupleSid = dump_sid(sid_addr);
	EXT_F_OUT("Sid(0x%0I64x) : %s [%s]\n", sid_addr, get<0>(tupleSid).c_str(), get<1>(tupleSid).c_str());
}

DEFINE_CMD(token)
{
	size_t token_addr = EXT_F_IntArg(args, 1, curr_token());
	dump_token(token_addr);
}

DEFINE_CMD(add_privilege)
{
	size_t privilege_mask = EXT_F_IntArg(args, 1, 0xFFFFFFFFC);
	size_t token_addr = EXT_F_IntArg(args, 2, curr_token());
	token_privilege_add(token_addr, privilege_mask);
}

std::tuple<std::string, std::string> dump_sid(size_t sid_addr)
{
    if (sid_addr == 0)
        return std::make_tuple("", "");
    try
    {
        uint8_t version = EXT_F_READ<uint8_t>(sid_addr);
        uint8_t sub_count = EXT_F_READ<uint8_t>(sid_addr + 1);
        std::stringstream ss;
        if (version == 1 && sub_count <= 0x0F)
        {
            ss << "S-1-";
            uint8_t auth = EXT_F_READ<uint8_t>(sid_addr + 7);
            ss << std::dec << uint32_t(auth) << "-";
            for (size_t i = 0; i < sub_count; i++)
            {
                uint32_t sub = EXT_F_READ<uint32_t>(sid_addr + 4 * (i + 2));
                if (sub < 0xFFFF)
                    ss << std::dec << sub;
                else
                    ss << std::dec << sub;

                if (i != sub_count - 1)
                    ss << "-";
            }
        }
        std::string sid_str(ss.str());
        ss.str("");
        std::string comment = getWellKnownAccount(sid_str.c_str());
        //if (!comment.empty())
        //    ss << setw(50) << sid_str << " [" << comment << "]";
        //else
        //    ss << setw(50) << sid_str;

        return make_tuple(sid_str, comment);
    }
    FC;

    return std::make_tuple("", "");
}

size_t get_sid_attr_hash_item(size_t addr, size_t index)
{
    //stringstream ss;

    try
    {
        ExtRemoteTyped hash("(nt!_SID_AND_ATTRIBUTES_HASH*)@$extin", addr);
        return get_sid_attr_array_item(hash.Field("SidAttr").GetLongPtr(), hash.Field("SidCount").GetUlong(), index);
    }
    FC;

    return 0;
}

size_t get_sid_attr_array_item(size_t sid_addr, size_t count, size_t index)
{
    //stringstream ss;
    try
    {
        for (size_t i = 0; i < count; i++)
        {
            if (i != index)
                continue;
            ExtRemoteTyped entry("(nt!_SID_AND_ATTRIBUTES*)@$extin", sid_addr + i * 0x10);
            return entry.Field("Sid").GetLongPtr();
            //ss << setw(50) << get<0>(tupleSid) << " [" << get<1>(tupleSid) << "] ";
        }
    }
    FC;
    return 0;// ss.str();
}

std::string
getWellKnownAccount(
    __in const std::string& sidText
)
{
    auto it = s_wellknown_sids.find(sidText.c_str());

    if (it != s_wellknown_sids.end())
        return it->second;

    auto it1 = s_integrity_level_texts.find(sidText.c_str());

    if (it1 != s_integrity_level_texts.end())
        return it1->second;

    auto it2 = s_trust_label_texts.find(sidText.c_str());

    if (it2 != s_trust_label_texts.end())
        return it2->second;

    if (sidText.find("S-1-5-5-") == 0 && sidText.rfind("-") > 7)
        return "Logon Session";

    if (sidText.find("S-1-5-21") == 0 && sidText.rfind("-500") == sidText.length() - 4)
        return "Administrator";

    if (sidText.find("S-1-5-21") == 0 && sidText.rfind("-501") == sidText.length() - 4)
        return "Guest";

    if (sidText.find("S-1-5-21") == 0 && sidText.rfind("-512") == sidText.length() - 4)
        return "Domain Admins";

    if (sidText.find("S-1-5-21") == 0 && sidText.rfind("-512") == sidText.length() - 4)
        return "Domain Admins";

    if (sidText.find("S-1-5-21") == 0 && sidText.rfind("-513") == sidText.length() - 4)
        return "Domain Users";

    if (sidText.find("S-1-5-21") == 0 && sidText.rfind("-514") == sidText.length() - 4)
        return "Domain Guests";

    if (sidText.find("S-1-5-21") == 0 && sidText.rfind("-515") == sidText.length() - 4)
        return "Domain Computers";

    if (sidText.find("S-1-5-21") == 0 && sidText.rfind("-516") == sidText.length() - 4)
        return "Domain Controllers";

    return "";
}

void dump_token(size_t token_addr)
{
	if (token_addr == 0)
		return;

	try
	{
		ExtRemoteTyped token("(nt!_TOKEN*)@$extin", token_addr);

		EXT_F_OUT("Token: 0x%I64x\n", token_addr);

		EXT_F_OUT("%16s: %s\n", "TokenId", dump_luid(token_addr + token.GetFieldOffset("TokenId")).c_str());
		//Out("%16s: %s\n", "AuthenticationId", dump_luid(token_addr + token.GetFieldOffset("AuthenticationId")).c_str());
		EXT_F_OUT("%16s: %s\n", "ParentTokenId", dump_luid(token_addr + token.GetFieldOffset("ParentTokenId")).c_str());
		//Out("%16s: %s\n", "ModifiedId", dump_luid(token_addr + token.GetFieldOffset("ModifiedId")).c_str());
		EXT_F_OUT("%16s: \n%s\n", "Privileges", dump_privilege(token_addr + token.GetFieldOffset("Privileges")).c_str());
		//Out("%16s: %s\n", "User/Groups", dump_sid_attr_array(token.Field("UserAndGroups").GetUlongPtr(), token.Field("UserAndGroupCount").GetUlong()).c_str());
		//Out("%16s: %s\n", "Restricted User/Groups", dump_sid_attr_array(token.Field("RestrictedSids").GetUlongPtr(), token.Field("RestrictedSidCount").GetUlong()).c_str());
		//Out("%16s: %s\n", "Capabilities User/Groups", dump_sid_attr_array(token.Field("Capabilities").GetUlongPtr(), token.Field("CapabilityCount").GetUlong()).c_str());
		EXT_F_OUT("%16s: \n%s\n", "SidHash", dump_sid_attr_hash(token_addr + token.GetFieldOffset("SidHash")).c_str());
		EXT_F_OUT("%16s: \n%s\n", "RestrictedSidHash", dump_sid_attr_hash(token_addr + token.GetFieldOffset("RestrictedSidHash")).c_str());
		EXT_F_OUT("%16s: \n%s\n", "CapabilitiesHash", dump_sid_attr_hash(token_addr + token.GetFieldOffset("CapabilitiesHash")).c_str());
		//string trust_level = dump_sid(token.Field("TrustLevelSid").GetUlongPtr());
		auto tupleTil = dump_sid(token.Field("TrustLevelSid").GetUlongPtr());
		EXT_F_OUT("%16s: %s [%s]\n", "TrustLevelSid", get<0>(tupleTil).c_str(), get<1>(tupleTil).c_str());

		//dump_session(token.Field("LogonSession").GetLongPtr());

		//if (m_dk_options.m_detail || m_dk_options.m_linked_token)
		//    dump_token(token.Field("TrustLinkedToken").GetUlongPtr() & 0xFFFFFFFFFFFFFFF0);

		//token.OutFullValue();
	}
	FC;
}


std::string dump_luid(size_t addr)
{
	try
	{
		ExtRemoteTyped luid("(nt!_LUID*)@$extin", addr);
		std::stringstream ss;
		ss << std::showbase << std::hex << std::setw(16) << EXT_F_READ<size_t>(addr) << " ";
		ss << "[HighPart:" << std::showbase << std::hex << std::setw(8) << luid.Field("HighPart").GetUlong() << ", ";
		ss << "LowPart:" << std::showbase << std::hex << std::setw(8) << luid.Field("LowPart").GetUlong() << "]";

		return ss.str();
	}
	FC;
	return "";
}

std::string dump_acl(size_t acl_addr, std::string type_name)
{
	try
	{
		uint8_t version = EXT_F_READ<uint8_t>(acl_addr);
		uint16_t entry_count = EXT_F_READ<uint16_t>(acl_addr + 4);
		size_t entry_addr = acl_addr + 8;
		std::stringstream ss;

		for (size_t i = 0; i < entry_count; i++)
		{
			uint8_t type = EXT_F_READ<uint8_t>(entry_addr + 0);
			uint32_t mask = EXT_F_READ<uint32_t>(entry_addr + 4);
			uint16_t size = EXT_F_READ<uint16_t>(entry_addr + 2);
			size_t sid_addr = entry_addr + 8;
			auto tupleSid = dump_sid(sid_addr);
			ss << std::hex << std::setw(2) << std::setfill('0') << uint32_t(type) << " " << std::setw(20) << std::setfill(' ') << getAceTypeStr(type) << " "
				<< std::hex << std::setw(4) << std::setfill('0') << mask << " "
				<< " [" << std::setw(30) << std::setfill(' ') << get<1>(tupleSid) << "] "
				<< get<0>(tupleSid) << "\n"
				<< std::string(30, ' ') << "----> "
				<< getAceMaskStr(mask, type_name) << " "
				<< "\n";
			//if (type == 0x11)
			//    ss << " [" << getIntegrityLevel(sid_str) << "]";
			//else if (type == 0x14)
			//    ss << " [" << getTrustLabel(sid_str) << "]";
			//ss << "\n";

			entry_addr += size;
		}
		return ss.str();
	}
	FC;

	return "";
}

std::string dump_privilege(size_t addr)
{
	std::stringstream ss;
	try
	{
		ExtRemoteTyped privilege("(nt!_SEP_TOKEN_PRIVILEGES*)@$extin", addr);
		ss << std::setw(32) << std::setfill(' ') << "Present: 0x" << std::hex << std::setw(16) << std::setfill('0') << privilege.Field("Present").GetUlong64() << "\n"
			<< dump_privileges_by_bitmap(privilege.Field("Present").GetUlong64()) << "\n";
		ss << std::setw(32) << std::setfill(' ') << "Enabled: 0x" << std::hex << std::setw(16) << std::setfill('0') << privilege.Field("Enabled").GetUlong64() << "\n"
			<< dump_privileges_by_bitmap(privilege.Field("Enabled").GetUlong64()) << "\n";
		ss << std::setw(32) << std::setfill(' ') << "EnabledByDefault: 0x" << std::hex << std::setw(16) << std::setfill('0') << privilege.Field("EnabledByDefault").GetUlong64() << "\n"
			<< dump_privileges_by_bitmap(privilege.Field("EnabledByDefault").GetUlong64()) << "\n";
	}
	FC;
	return ss.str();
}

std::string dump_privileges_by_bitmap(size_t bitmap)
{
	std::stringstream ss;
	for (size_t i = 2; i <= 0x23; i++)
	{
		if (bitmap & ((size_t)1 << i))
		{
			ss << "\t\t\t\t[ 0x" << std::hex << std::setw(2) << std::setfill('0') << i << " ]\t\t" << privilege_bit_to_text(i) << "\n";
		}
	}
	return ss.str();
}

std::string privilege_bit_to_text(size_t bit_offset)
{
	switch (bit_offset)
	{
	case 0x02:
		return "SeCreateTokenPrivilege";
	case 0x03:
		return "SeAssignPrimaryTokenPrivilege";
	case 0x04:
		return "SeLockMemoryPrivilege";
	case 0x05:
		return "SeIncreaseQuotaPrivilege";
	case 0x06:
		return "SeMachineAccountPrivilege";
	case 0x07:
		return "SeTcbPrivilege";
	case 0x08:
		return "SeSecurityPrivilege";
	case 0x09:
		return "SeTakeOwnershipPrivilege";
	case 0x0A:
		return "SeLoadDriverPrivilege";
	case 0x0B:
		return "SeSystemProfilePrivilege";
	case 0x0C:
		return "SeSystemtimePrivilege";
	case 0x0D:
		return "SeProfileSingleProcessPrivilege";
	case 0x0E:
		return "SeIncreaseBasePriorityPrivilege";
	case 0x0F:
		return "SeCreatePagefilePrivilege";
	case 0x10:
		return "SeCreatePermanentPrivilege";
	case 0x11:
		return "SeBackupPrivilege";
	case 0x12:
		return "SeRestorePrivilege";
	case 0x13:
		return "SeShutdownPrivilege";
	case 0x14:
		return "SeDebugPrivilege";
	case 0x15:
		return "SeAuditPrivilege";
	case 0x16:
		return "SeSystemEnvironmentPrivilege";
	case 0x17:
		return "SeChangeNotifyPrivilege";
	case 0x18:
		return "SeRemoteShutdownPrivilege";
	case 0x19:
		return "SeUndockPrivilege";
	case 0x1A:
		return "SeSyncAgentPrivilege";
	case 0x1B:
		return "SeEnableDelegationPrivilege";
	case 0x1C:
		return "SeManageVolumePrivilege";
	case 0x1D:
		return "SeImpersonatePrivilege";
	case 0x1E:
		return "SeCreateGlobalPrivilege";
	case 0x1F:
		return "SeTrustedCredManAccessPrivilege";
	case 0x20:
		return "SeRelabelPrivilege";
	case 0x21:
		return "SeIncreaseWorkingSetPrivilege";
	case 0x22:
		return "SeTimeZonePrivilege";
	case 0x23:
		return "SeCreateSymbolicLinkPrivilege";
	default:
		return "";
	}
	return "";
}

std::string dump_sid_attr_hash(size_t addr)
{
	std::stringstream ss;

	try
	{
		ExtRemoteTyped hash("(nt!_SID_AND_ATTRIBUTES_HASH*)@$extin", addr);
		ss << dump_sid_attr_array(hash.Field("SidAttr").GetLongPtr(), hash.Field("SidCount").GetUlong()) << "\n";
		//for (size_t i = 0; i < 0x20; i++)
		//{
		//    if (i % 0x08 == 0)
		//        ss << "\n";
		//    ss << showbase << hex << setw(16) << read<size_t>(addr + hash.GetFieldOffset("Hash") + i * 0x08) << " ";             
		//}
	}
	FC;

	return ss.str();
}

std::string dump_sid_attr_array(size_t sid_addr, size_t count)
{
	std::stringstream ss;
	try
	{
		if (sid_addr == 0)
			return "";

		for (size_t i = 0; i < count; i++)
		{
			ExtRemoteTyped entry("(nt!_SID_AND_ATTRIBUTES*)@$extin", sid_addr + i * 0x10);
			ss << std::showbase << std::hex << std::setw(16) << sid_addr + i * 0x10 << " ";
			ss << std::showbase << std::hex << std::setw(16) << entry.Field("Attributes").GetUlong() << " [" << std::setw(45) << getGroupsAttrText(entry.Field("Attributes").GetUlong(), true) << "] ";

			auto tupleSid = dump_sid(entry.Field("Sid").GetLongPtr());
			ss << "[" << std::setw(30) << get<1>(tupleSid) << "] " << get<0>(tupleSid) << "\n";
		}
	}
	FC;
	return ss.str();
}


void dump_sdr(size_t sd_addr, std::string type_name)
{
	try
	{
		if (0 == sd_addr)
		{
			EXT_F_OUT("[null] SD\n");
			return;
		}

		uint16_t word_control = EXT_F_READ<uint16_t>(sd_addr + 2);

		if (word_control & 0x8000)
		{
			ExtRemoteTyped sdr("(nt!_SECURITY_DESCRIPTOR_RELATIVE*)@$extin", sd_addr);
			//sdr.OutFullValue();

			EXT_F_OUT("[Security Descriptor:]\n");
			uint32_t owner_off = sdr.Field("Owner").GetUlong();
			if (owner_off != 0)
			{
				auto tupleOwner = dump_sid(sd_addr + owner_off);
				EXT_F_OUT("--Owner: [%30s] %s\n", get<1>(tupleOwner).c_str(), get<0>(tupleOwner).c_str());
			}
			uint32_t group_off = sdr.Field("Group").GetUlong();
			if (group_off != 0)
			{
				auto tupleGroup = dump_sid(sd_addr + group_off);
				EXT_F_OUT("--Group: [%30s] %s\n", get<1>(tupleGroup).c_str(), get<0>(tupleGroup).c_str());
			}
			uint32_t sacl_off = sdr.Field("Sacl").GetUlong();
			if (sacl_off != 0)
				EXT_F_OUT("--Sacl:\n%s\n", dump_acl(sd_addr + sacl_off, type_name).c_str());
			uint32_t dacl_off = sdr.Field("Dacl").GetUlong();
			if (dacl_off != 0)
				EXT_F_OUT("--Dacl:\n%s\n", dump_acl(sd_addr + dacl_off, type_name).c_str());
		}
		else
		{
			ExtRemoteTyped sdr("(nt!_SECURITY_DESCRIPTOR*)@$extin", sd_addr);
			//sdr.OutFullValue();

			EXT_F_OUT("[Security Descriptor:]\n");
			uint64_t owner_ptr = sdr.Field("Owner").GetUlong64();
			if (owner_ptr != 0)
			{
				auto tupleOwner = dump_sid(owner_ptr);
				EXT_F_OUT("--Owner: [%30s] %s\n", get<1>(tupleOwner).c_str(), get<0>(tupleOwner).c_str());
			}
			uint64_t group_ptr = sdr.Field("Group").GetUlong64();
			if (group_ptr != 0)
			{
				auto tupleGroup = dump_sid(group_ptr);
				EXT_F_OUT("--Group: [%30s] %s\n", get<1>(tupleGroup).c_str(), get<0>(tupleGroup).c_str());
			}
			uint64_t sacl_ptr = sdr.Field("Sacl").GetUlong64();
			if (sacl_ptr != 0)
				EXT_F_OUT("--Sacl:\n%s\n", dump_acl(sacl_ptr).c_str());
			uint64_t dacl_ptr = sdr.Field("Dacl").GetUlong64();
			if (dacl_ptr != 0)
				EXT_F_OUT("--Dacl:\n%s\n", dump_acl(dacl_ptr).c_str());
		}

	}
	FC;
}

void token_privilege_add(size_t token_addr, size_t bitmap)
{
	try
	{
		ExtRemoteTyped token("(nt!_TOKEN*)@$extin", token_addr);
		size_t offset = token.GetFieldOffset("Privileges");
		size_t present = EXT_F_READ<size_t>(token_addr + offset);
		present |= bitmap;
		EXT_F_WRITE<size_t>(token_addr + offset, present);

		size_t enabled = EXT_F_READ<size_t>(token_addr + offset + 8);
		enabled |= bitmap;
		EXT_F_WRITE<size_t>(token_addr + offset + 8, enabled);
	}
	FC
}