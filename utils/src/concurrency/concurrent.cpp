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

#include "concurrent.h"

using namespace Halley;

#ifdef _MSC_VER
#pragma warning(disable: 4100)
#endif

#include <boost/thread.hpp>
static boost::thread_specific_ptr<String> threadName;

#ifdef _WIN32
#include <windows.h>
const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
#pragma warning(disable: 6320 6322)

void SetThreadName( DWORD dwThreadID, const char* name)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

#pragma warning(default: 6320 6322)
#endif

namespace Halley {
namespace Concurrent {

	void setThreadName(String name)
	{
#ifdef _WIN32
#ifdef _DEBUG
		SetThreadName((DWORD)-1, name.c_str());
#endif
#endif
		threadName.reset(new String(name));
	}

	String getThreadName()
	{
		String *name = threadName.get();
		if (name) return *name;

		std::stringstream ss;
		ss << boost::this_thread::get_id();

		String n = String("thread_") + ss.str();
		setThreadName(n);
		return n;
	}

}
}
