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

#pragma once

#include "halley/text/halleystring.h"
#include <list>
#include <array>
#include <functional>
#include <atomic>

#include "halley/concurrency/mutex.h"

namespace Halley {
	struct DebugTraceEntry
	{
		const char* filename = nullptr;
		int line = 0;
		std::array<char, 244> arg = {};
	};

	class Debug {
	public:
		static constexpr bool isDebug()
		{
		#ifdef _DEBUG
			return true;
		#else
			return false;
		#endif
		}

		static void setErrorHandling(const String& dumpFilePath, std::function<void(std::string_view)> errorHandler);
		static String getCallStack(int skip = 3); // Thread safe
		static void getCallStack(gsl::span<char> dst, int skip = 3); // Not thread safe, use in unsafe environments
		static void printCallStackTo(std::ostream& out, int skip);

		static void trace(const char* filename, int line, std::string_view arg = {});
		static String getLastTraces();
		static void printLastTraces();

		static void abort();
		static void abort(std::string_view message);

		static bool isRunningFromDLL();

	private:
		Debug();
		static bool debugging;
		static std::array<DebugTraceEntry, 32> lastTraces;
		static std::atomic<int> tracePos;
		static Mutex mutex;
	};

#if defined(DEV_BUILD) || defined(_DEBUG)
	#define HALLEY_DEBUG_TRACE() Halley::Debug::trace(__FILE__, __LINE__)
	#define HALLEY_DEBUG_TRACE_COMMENT(str) Halley::Debug::trace(__FILE__, __LINE__, (str))
	#define HALLEY_DEBUG_TRACE_THIS() Halley::Debug::trace(__FILE__, __LINE__, typeid(*this).name())
#else
	#define HALLEY_DEBUG_TRACE() (static_cast<void>(0))
	#define HALLEY_DEBUG_TRACE_COMMENT(str) (static_cast<void>(0))
	#define HALLEY_DEBUG_TRACE_THIS() (static_cast<void>(0))
#endif
}
