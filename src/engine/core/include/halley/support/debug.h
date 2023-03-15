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

namespace Halley {
	struct DebugTraceEntry
	{
		const char* filename = nullptr;
		int line = 0;
		std::array<char, 256> arg = {};
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

		static void setErrorHandling(const String& dumpFilePath, std::function<void(const std::string&)> errorHandler);
		static String getCallStack(int skip = 3);

		static void trace(const char* filename, int line, const char* arg = nullptr);
		static String getLastTraces();
		static void printLastTraces();

	private:
		Debug();
		static bool debugging;
		static std::array<DebugTraceEntry, 16> lastTraces;
		static int tracePos;
	};

	#define HALLEY_DEBUG_TRACE() Halley::Debug::trace(__FILE__, __LINE__)
	#define HALLEY_DEBUG_TRACE_COMMENT(str) Halley::Debug::trace(__FILE__, __LINE__, (str))
	#define HALLEY_DEBUG_TRACE_THIS() Halley::Debug::trace(__FILE__, __LINE__, typeid(*this).name())
}
