/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include "debug.h"
#include "console.h"
#include "../text/halleystring.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include "../os/os.h"

using namespace Halley;

bool Halley::Debug::debugging = false;

void Halley::Debug::setDebugMode(bool enabled)
{
	debugging = enabled;
}

void Halley::Debug::toggleDebugMode()
{
	debugging = !debugging;
}

bool Halley::Debug::isDebugMode()
{
	return debugging;
}

Halley::Debug::Debug()
{
}


////////


#ifdef _MSC_VER
#include "StackWalker/StackWalker.h"
#include <dbghelp.h> 
#include <tchar.h>

#pragma comment(lib, "dbghelp.lib")
#pragma warning(disable: 4996)

class CoutStackWalker : public StackWalker
{
public:
	CoutStackWalker()
		: StackWalker()
	{
	}

protected:
	void OnOutput(LPCSTR text)
	{
		std::cout << text;
		StackWalker::OnOutput(text);
	}
};

class StringStackWalker : public StackWalker
{
public:
	StringStackWalker(std::stringstream& s, size_t skipFrames=0)
		: StackWalker(StackWalker::RetrieveSymbol | StackWalker::RetrieveLine | StackWalker::SymAll)
		, str(s)
		, skip(skipFrames)
	{
	}

	StringStackWalker& operator=(const StringStackWalker&) = delete;

protected:
	void OnLoadModule(LPCSTR, LPCSTR, DWORD64, DWORD, DWORD, LPCSTR, LPCSTR, ULONGLONG)
	{
	}

	void OnSymInit(LPCSTR, DWORD, LPCSTR)
	{
	}

	void OnOutput(LPCSTR text)
	{
		if (skip > 0) {
			skip--;
		} else {
			str << "\t" << text;
			StackWalker::OnOutput(text);
		}
	}

	std::stringstream& str;
	size_t skip;
};


///////////////////////////////////////////////////////////////////////////////
// This function determines whether we need data sections of the given module 
bool IsDataSectionNeeded(const WCHAR* pModuleName) 
{
	// Check parameters 
	if( pModuleName == 0 ) {
		_ASSERTE( _T("Parameter is null.") ); 
		return false; 
	}

	// Extract the module name 
	WCHAR szFileName[_MAX_FNAME] = L""; 
	_wsplitpath( pModuleName, NULL, NULL, szFileName, NULL ); 

	// Compare the name with the list of known names and decide 
	// Note: For this to work, the executable name must be "mididump.exe"
	if(wcsicmp( szFileName, L"ggj11" ) == 0) {
		return true; 
	}
	else if(wcsicmp( szFileName, L"ntdll" ) == 0) {
		return true; 
	}

	return false; 
}


///////////////////////////////////////////////////////////////////////////////
// Custom minidump callback 
//

BOOL CALLBACK MyMiniDumpCallback(PVOID /*pParam*/, const PMINIDUMP_CALLBACK_INPUT pInput, PMINIDUMP_CALLBACK_OUTPUT pOutput) 
{
	BOOL bRet = FALSE; 

	// Check parameters 
	if( pInput == 0 ) return FALSE; 
	if( pOutput == 0 ) return FALSE; 
	
	// Process the callbacks 
	switch( pInput->CallbackType ) {
	case IncludeModuleCallback: 
		{
			// Include the module into the dump 
			bRet = TRUE; 
		}
		break; 

	case IncludeThreadCallback: 
		{
			// Include the thread into the dump 
			bRet = TRUE; 
		}
		break; 

	case ModuleCallback: 
		{
			// Are data sections available for this module ? 

			if( pOutput->ModuleWriteFlags & ModuleWriteDataSeg ) 
			{
				// Yes, they are, but do we need them? 

				if( !IsDataSectionNeeded( pInput->Module.FullPath ) ) 
				{
					wprintf( L"Excluding module data sections: %s \n", pInput->Module.FullPath ); 

					pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg); 
				}
			}

			bRet = TRUE; 
		}
		break; 

	case ThreadCallback: 
		{
			// Include all thread information into the minidump 
			bRet = TRUE;  
		}
		break; 

	case ThreadExCallback: 
		{
			// Include this information 
			bRet = TRUE;  
		}
		break; 

	case MemoryCallback: 
		{
			// We do not include any information here -> return FALSE 
			bRet = FALSE; 
		}
		break; 

	case CancelCallback: 
		break; 
	}

	return bRet; 

}


void CreateMiniDump( EXCEPTION_POINTERS* pep, LPCWSTR path) 
{
	// Open the file 

	HANDLE hFile = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 

	if((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE)) {
		// Create the minidump 
		MINIDUMP_EXCEPTION_INFORMATION mdei; 
		mdei.ThreadId           = GetCurrentThreadId(); 
		mdei.ExceptionPointers  = pep; 
		mdei.ClientPointers     = FALSE; 

		MINIDUMP_CALLBACK_INFORMATION mci; 
		mci.CallbackRoutine     = (MINIDUMP_CALLBACK_ROUTINE)MyMiniDumpCallback; 
		mci.CallbackParam       = 0; 

		//MINIDUMP_TYPE mdt       = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory | MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules ); 
		MINIDUMP_TYPE mdt       = (MINIDUMP_TYPE)(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory); 

		BOOL rv = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, mdt, (pep != 0) ? &mdei : 0, 0, &mci); 

		if(!rv) std::cout << "Failed creating minidump, last error = " << GetLastError() << std::endl; 
		else std::cout << "Minidump created successfully." << std::endl;

		CloseHandle(hFile); 
	}
	else {
		std::cout << "Failed creating file for minidump, last error = " << GetLastError() << std::endl;
	}

}


#ifndef _DEBUG
void printException(PEXCEPTION_RECORD record)
{
	String code = "unknown";
	int c = record->ExceptionCode;

	switch (c) {
	case EXCEPTION_ACCESS_VIOLATION: code = "Access violation"; break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: code = "Array bounds exceeded"; break;
	case EXCEPTION_BREAKPOINT: code = "Breakpoint"; break;
	case EXCEPTION_DATATYPE_MISALIGNMENT: code = "Datatype misalignment"; break;
	case EXCEPTION_FLT_DENORMAL_OPERAND: code = "Float denormal operand"; break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO: code = "Float divide by zero"; break;
	case EXCEPTION_FLT_INEXACT_RESULT: code = "Float inexact result"; break;
	case EXCEPTION_FLT_INVALID_OPERATION: code = "Flt invalid operation"; break;
	case EXCEPTION_FLT_OVERFLOW: code = "Float overflow"; break;
	case EXCEPTION_FLT_STACK_CHECK: code = "Float stack check"; break;
	case EXCEPTION_FLT_UNDERFLOW: code = "Float underflow"; break;
	case EXCEPTION_ILLEGAL_INSTRUCTION: code = "Illegal instruction"; break;
	case EXCEPTION_IN_PAGE_ERROR: code = "In page error"; break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO: code = "Integer divide by zero"; break;
	case EXCEPTION_INT_OVERFLOW: code = "Integer overflow"; break;
	case EXCEPTION_INVALID_DISPOSITION: code = "Invalid disposition"; break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: code = "Noncontinuable exception"; break;
	case EXCEPTION_PRIV_INSTRUCTION: code = "Privileged instruction"; break;
	case EXCEPTION_SINGLE_STEP: code = "Single step"; break;
	case EXCEPTION_STACK_OVERFLOW: code = "Stack overflow";
	}

	std::cout << "Unhandled exception: " << code;
	std::cout << " at 0x" << record->ExceptionAddress;

	if (c == EXCEPTION_ACCESS_VIOLATION || c == EXCEPTION_IN_PAGE_ERROR) {
		switch (record->ExceptionInformation[0]) {
		case 0: std::cout << ", reading"; break;
		case 1: std::cout << ", writing"; break;
		case 8: std::cout << ", DEP on"; break;
		default: std::cout << ", unknown on";
		}
		std::cout << " address: 0x";
		std::cout << (void*) record->ExceptionInformation[1];

		if (c == EXCEPTION_IN_PAGE_ERROR) std::cout << ", NTSTATUS: " << record->ExceptionInformation[2];
	}

	std::cout << std::endl;
}


LONG onUnhandledException(LPEXCEPTION_POINTERS e)
{
#ifdef _M_IX86 
	if (e->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)   
	{ 
		// be sure that we have enough space... 
		static char MyStack[1024*128];   
		// it assumes that DS and SS are the same!!! (this is the case for Win32) 
		// change the stack only if the selectors are the same (this is the case for Win32) 
		//__asm push offset MyStack[1024*128]; 
		//__asm pop esp; 
		__asm mov eax,offset MyStack[1024*128]; 
		__asm mov esp,eax; 
	} 
#endif

	time_t rawtime;
	time(&rawtime);
	char buffer[128];
	strftime (buffer, 100, "%Y-%m-%d-%H-%M-%S", localtime(&rawtime));
	String path = OS::get().getUserDataDir() + "minidump-" + buffer + ".dmp";
	std::cout << "Writing minidump to \"" << path << "\"..." << std::endl;
	CreateMiniDump(e, path.getUTF16().c_str());

	printException(e->ExceptionRecord);
	std::cout << "===== CALL STACK =====" << std::endl;
	CoutStackWalker walker;
	walker.ShowCallstack(GetCurrentThread(), e->ContextRecord);
	std::cout << "======================" << std::endl;
	std::cout << "Last trace: " << Debug::getLastTraces() << std::endl;
	return EXCEPTION_EXECUTE_HANDLER;
}

//#ifndef _M_IX86
//#error "The following code only works for x86!"
//#endif
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI MyDummySetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER /*lpTopLevelExceptionFilter*/)
{
	return NULL;
}

BOOL PreventSetUnhandledExceptionFilter()
{
	HMODULE hKernel32 = LoadLibrary(L"kernel32.dll");
	if (hKernel32 == NULL) return FALSE;
	void *pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
	if(pOrgEntry == NULL) return FALSE;
	unsigned char newJump[ 100 ];
	size_t dwOrgEntryAddr = (size_t) pOrgEntry;
	dwOrgEntryAddr += 5; // add 5 for 5 op-codes for jmp far
	void *pNewFunc = &MyDummySetUnhandledExceptionFilter;
	size_t dwNewEntryAddr = (size_t) pNewFunc;
	size_t dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;

	newJump[ 0 ] = 0xE9;  // JMP absolute
	memcpy(&newJump[ 1 ], &dwRelativeAddr, sizeof(pNewFunc));
	SIZE_T bytesWritten;
	BOOL bRet = WriteProcessMemory(GetCurrentProcess(),
		pOrgEntry, newJump, sizeof(pNewFunc) + 1, &bytesWritten);
	return bRet;
}
#endif

#endif

void Halley::Debug::setErrorHandling()
{
#ifdef _WIN32
#ifndef _DEBUG
	//SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER) onUnhandledException);
	//PreventSetUnhandledExceptionFilter();
#endif
#endif
}

void Halley::Debug::printCallStack()
{
#ifdef _WIN32
	std::cout << ConsoleColor(Console::RED) << "===== CALL STACK =====" << ConsoleColor(Console::DARK_RED) << std::endl;
	CoutStackWalker walker;
	walker.ShowCallstack();
	std::cout << ConsoleColor(Console::RED) << "======================" << ConsoleColor() << std::endl;
#endif
}

Halley::String Halley::Debug::getCallStack()
{
#ifdef _WIN32
	std::stringstream str;
	StringStackWalker walker(str, 3);
	walker.ShowCallstack();
	return str.str();
#else
	return "";
#endif
}

void Halley::Debug::trace(String str)
{
	if (lastTraces.size() >= 5) lastTraces.pop_back();
	lastTraces.push_front(str);
}

Halley::String Halley::Debug::getLastTraces()
{
	String result;
	for (auto i=lastTraces.begin(); i != lastTraces.end(); i++) {
		result += " - " + (*i) + "\n";
	}
	return result;
}

std::list<Halley::String> Halley::Debug::lastTraces;
