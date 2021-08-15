#include "DynWAPI.h"
#include "signatureFunc.h"

namespace dynWAPI
{
	LdrGetProcedureAddress pLdrGetProcedureAddress;
	PLdrLoadDll            pLdrLoadDll;
	HMODULE                hNtdll;
	PMODULE_INF            arrModuleInf;
	size_t                 countModules;

	/*
		This function get address of function: LdrLoadDll, pLdrGetProcedureAddress
		and handle of NTDLL
		@ret = DWAPI_STATUS 
	*/
	DWAPI_STATUS init(PMODULE_INF pModuleInf, size_t count) {
		DWAPI_STATUS status = DWAPI_STATUS::ERROR_INVALID_PARAM;

		if (pModuleInf) {

			// Get ntdll handle
			hNtdll = getHandleNtDll();

			if (hNtdll) {
				
				// Get LdrLoadDll func address
				pLdrLoadDll = static_cast<PLdrLoadDll>(getFuncAddrFromNtdll(STR_LDRLOADDLL));

				if (pLdrLoadDll) {

					// Get LdrGetProcedureAddress func address
					pLdrGetProcedureAddress = static_cast<LdrGetProcedureAddress>
						(getFuncAddrFromNtdll(STR_LDRGETPROCEDUREADDRESS));

					if (pLdrGetProcedureAddress)
						status = loadModules(pModuleInf, count);
					else
						status = DWAPI_STATUS::ERROR_FIND_LDRGETPROCEDUREADDRESS;
				}
				else
					status = DWAPI_STATUS::ERROR_FIND_LDRLOADDLL;
			}
			else
				status = DWAPI_STATUS::ERROR_NTDLL_HANDLE;
		}

		if (DWAPI_SUCCESS(status)) {
			arrModuleInf = pModuleInf;
			countModules = count;
		}

		return status;
	}

	/*
		- Get handle of a ntdll.dll
		- Loaded dynamic link library
		@ret = DWAPI_STATUS 
	*/
	DWAPI_STATUS loadModules(PMODULE_INF pModuleInf, size_t count) {

		for (size_t i = 0; i < count; ++i) {
			pModuleInf[i].hLib = loadLibrary(pModuleInf[i].moduleName);
			
			if (!pModuleInf[i].hLib)
				return DWAPI_STATUS::ERROR_LOAD_LIB;
		}

		return DWAPI_STATUS::SUCCESS;
	}

	/*
		Loading dynamic library using native wndows-api function 'LdrLoadDll'
		@dllName - dynamic library name
		@ret = handle of a dll
	*/
	HMODULE loadLibrary(const wchar_t* dllName) {
		HMODULE outDllHandle = 0;

		if (dllName) {

			// Init string
			auto dllNameSize = stringSizeBytes(dllName);
			UNICODE_STRING UnicodeDllName;
			UnicodeDllName.Buffer = const_cast<wchar_t*> (dllName);
			UnicodeDllName.Length = static_cast<USHORT>(dllNameSize);
			UnicodeDllName.MaximumLength = static_cast<USHORT>(
				(dllNameSize) + sizeof(wchar_t)
				);

			// Get handle of dll
			pLdrLoadDll(nullptr, 0, &UnicodeDllName, &outDllHandle);
		}

		return outDllHandle;
	}

	/*
		This function gets the handle to the dynamic link library 'ntdll.dll' from PEB
		@ret = handle ntdll.dll
	*/
	HMODULE getHandleNtDll() {

		HMODULE handleNtdll = 0;

		PPEB ptrPeb = reinterpret_cast<PPEB>(__readgsqword(PEB_OFFSET));
		if (ptrPeb) {
			pProcessModuleInfo ProcessModule = reinterpret_cast<pProcessModuleInfo>(ptrPeb->Ldr);
			pModuleInfoNode    ModuleList    = reinterpret_cast<pModuleInfoNode>(ProcessModule->ModuleListLoadOrder.Flink);
			
			// Finding dlls trought the PEB
			while (ModuleList->BaseAddress) {

				// Find NTDLL.dll
				if (compareString((wchar_t*)ModuleList->BaseDllName.Buffer, (wchar_t*)STR_NTDLL))
				{
					handleNtdll = static_cast<HMODULE>(ModuleList->BaseAddress);
					break;
				}

				ModuleList = reinterpret_cast<pModuleInfoNode>(ModuleList->InLoadOrderModuleList.Flink);
			}
		}

		return handleNtdll;
	}

	/*
	Get handle dll by name
	@ret = dll handle
*/
	HMODULE getHandleModuleByName(const wchar_t* dllName) {

		for (decltype(countModules) i = 0; i < countModules; ++i) {
			if (compareString((const wchar_t*)arrModuleInf[i].moduleName, dllName))
				return arrModuleInf[i].hLib;
		}

		return 0;
	}

	/*
		Find the address of a function from a NTDLL
		@hModule - handle to the loaded dynamic library
		@functionName - name function
		@ret = address of a function
	*/
	LPVOID getFuncAddrFromNtdll(const char* functionName) {
		
		LPVOID reusltAddrFunc = nullptr;

		if (functionName) {
			PIMAGE_DOS_HEADER       imgDosHeader        = nullptr;
			PIMAGE_NT_HEADERS       imgNtheader         = nullptr;
			PIMAGE_EXPORT_DIRECTORY exportSection       = nullptr;
			PDWORD                  nameExpFuncBaseAddr = nullptr;
			PWORD                   rvaOrdinal          = nullptr;
			int ordinal                                 = -1;

			imgDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hNtdll);
			if (imgDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
				goto EXIT;

			imgNtheader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<DWORD_PTR>(imgDosHeader) + imgDosHeader->e_lfanew);
			if (imgNtheader->Signature != IMAGE_NT_SIGNATURE)
				goto EXIT;

			if (
				(imgNtheader->FileHeader.Characteristics & IMAGE_FILE_DLL) == NULL ||
				imgNtheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress == NULL ||
				imgNtheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size == NULL
				)
				goto EXIT;

			exportSection = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(reinterpret_cast<DWORD_PTR>(imgDosHeader) +
				imgNtheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

			if (exportSection == nullptr)
				goto EXIT;

			nameExpFuncBaseAddr = reinterpret_cast<PDWORD>(reinterpret_cast<DWORD_PTR>(imgDosHeader) +
				exportSection->AddressOfNames);

			if (nameExpFuncBaseAddr == nullptr)
				goto EXIT;

			rvaOrdinal = reinterpret_cast<PWORD>(reinterpret_cast<DWORD_PTR>(imgDosHeader) +
				exportSection->AddressOfNameOrdinals);

			if (rvaOrdinal == nullptr)
				goto EXIT;

			// Function search
			for (ULONG i = 0; i < exportSection->NumberOfNames; ++i) {
				PCHAR api_name = reinterpret_cast<PCHAR>(reinterpret_cast<DWORD_PTR>(imgDosHeader) + nameExpFuncBaseAddr[i]);

				if (compareString((PCHAR)api_name, (PCHAR)functionName)) {
					ordinal = static_cast<UINT>(rvaOrdinal[i]);
					break;
				}
			}

		EXIT:
			if (ordinal != -1) {
				PDWORD offsetFunc = reinterpret_cast<PDWORD>(reinterpret_cast<DWORD_PTR>(imgDosHeader) +
					exportSection->AddressOfFunctions);
				reusltAddrFunc = reinterpret_cast<LPVOID>(reinterpret_cast<DWORD_PTR>(imgDosHeader) + offsetFunc[ordinal]);
			}

		}
		
		return reusltAddrFunc;
	}

	/*
		
	*/
	PVOID getProcAddress(HMODULE hmodule,const char* functionName)
	{
		PVOID outFunAddr = nullptr;

		if (functionName) {

			// Init string
			auto fNameLen = stringSizeBytes(functionName);
			ANSI_STRING ansiStringName;
			ansiStringName.Buffer = const_cast<char*> (functionName);
			ansiStringName.Length = static_cast<USHORT>(fNameLen);
			ansiStringName.MaximumLength = static_cast<USHORT>(
				(fNameLen) + sizeof(wchar_t)
				);

			// Get handle of dll
			pLdrGetProcedureAddress(hmodule, &ansiStringName, 0, &outFunAddr);
		}

		return outFunAddr;
	}

	/*
		This function free all allocated resources
		@ret = void
	*/
	void destroy() {
		// empty
	}
}