#pragma once
#include <Windows.h>
#include <subauth.h>
#include "DynWAPI.h"
#include "utils.h"

/*
	The module for dynamically obtaining the function of Windows
*/

#define PEB_OFFSET                  0x60
#define STR_NTDLL                   L"ntdll.dll"
#define STR_LDRLOADDLL              "LdrLoadDll"
#define STR_LDRGETPROCEDUREADDRESS  "LdrGetProcedureAddress"

#define MAX_LEN_NAME 256

#define DWAPI_SUCCESS(dwapiStatus) (dwapiStatus==DWAPI_STATUS::SUCCESS)
#define DWAPI_ERROR(dwapiStatus) !DWAPI_SUCCESS(dwapiStatus)

#define COUNT_OF(array) (sizeof(array)/sizeof(array[0]))

enum class DWAPI_STATUS :unsigned int {
	SUCCESS = 0,
	ERROR_LOAD_LIB,
	ERROR_NO_LIBS,
	ERROR_INVALID_PARAM,
	ERROR_NTDLL_HANDLE,
	ERROR_FIND_LDRLOADDLL,
	ERROR_FIND_LDRGETPROCEDUREADDRESS,
};

typedef struct _MODULE_INF {
	WCHAR moduleName[MAX_LEN_NAME];
	HMODULE hLib;
} MODULE_INF, *PMODULE_INF;

namespace dynWAPI
{
	typedef struct _UNICODE_STRING
	{
		USHORT Length;
		USHORT MaximumLength;
		PWSTR Buffer;
	} UNICODE_STRING, * PUNICODE_STRING;

	typedef struct _STRING {
		USHORT Length;
		USHORT MaximumLength;
		PCHAR  Buffer;
	} STRING;

	typedef STRING ANSI_STRING, *PANSI_STRING;

	typedef struct _ModuleInfoNode {

		LIST_ENTRY              InLoadOrderModuleList;
		LIST_ENTRY              InMemoryOrderModuleList;
		LIST_ENTRY              InInitializationOrderModuleList;
		PVOID                   BaseAddress;
		PVOID                   EntryPoint;
		ULONG                   SizeOfImage;
		UNICODE_STRING          FullDllName;
		UNICODE_STRING          BaseDllName;
		ULONG                   Flags;
		SHORT                   LoadCount;
		SHORT                   TlsIndex;
		LIST_ENTRY              HashTableEntry;
		ULONG                   TimeDateStamp;

	} ModuleInfoNode, * pModuleInfoNode;

	typedef struct _ProcessModuleInfo {
		/*000*/  ULONG Length;
		/*004*/  BOOLEAN Initialized;
		/*008*/  PVOID SsHandle;
		/*00C*/  LIST_ENTRY ModuleListLoadOrder;
		/*014*/  LIST_ENTRY ModuleListMemoryOrder;
		/*018*/  LIST_ENTRY ModuleListInitOrder;
		/*020*/
	} ProcessModuleInfo, * pProcessModuleInfo;

	typedef struct _PEB_LDR_DATA {
		BYTE Reserved1[8];
		PVOID Reserved2[3];
		LIST_ENTRY InMemoryOrderModuleList;
	} PEB_LDR_DATA, * PPEB_LDR_DATA;

	typedef struct _RTL_USER_PROCESS_PARAMETERS {
		BYTE Reserved1[16];
		PVOID Reserved2[10];
		UNICODE_STRING ImagePathName;
		UNICODE_STRING CommandLine;
	} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS;

	using PPS_POST_PROCESS_INIT_ROUTINE =  VOID (NTAPI*)(VOID);

	typedef struct _PEB {
		BYTE Reserved1[2];
		BYTE BeingDebugged;
		BYTE Reserved2[1];
		PVOID Reserved3[2];
		PPEB_LDR_DATA Ldr;
		PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
		PVOID Reserved4[3];
		PVOID AtlThunkSListPtr;
		PVOID Reserved5;
		ULONG Reserved6;
		PVOID Reserved7;
		ULONG Reserved8;
		ULONG AtlThunkSListPtr32;
		PVOID Reserved9[45];
		BYTE Reserved10[96];
		PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
		BYTE Reserved11[128];
		PVOID Reserved12[1];
		ULONG SessionId;
	} PEB, * PPEB;

	using PLdrLoadDll = NTSTATUS(*)(PWCHAR, ULONG, PUNICODE_STRING, HMODULE*);
	using LdrGetProcedureAddress = NTSTATUS(*)(IN HMODULE ModuleHandle, IN PANSI_STRING FunctionName OPTIONAL,WORD Oridinal OPTIONAL, PVOID* FunctionAddress );

	HMODULE getHandleNtDll();

	DWAPI_STATUS init(PMODULE_INF pModuleInf, size_t count);

	DWAPI_STATUS loadModules(PMODULE_INF pModuleInf, size_t count);

	HMODULE getHandleModuleByName(const wchar_t* dllName);

	LPVOID getFuncAddrFromNtdll(const char* functionName);

	HMODULE loadLibrary(const wchar_t* dllName);

	PVOID getProcAddress(HMODULE hmodule, const char* functionName);

};

#define DWAPI(dll, func) ((_##func)dynWAPI::getProcAddress(dynWAPI::getHandleModuleByName(##dll), (char*)#func))