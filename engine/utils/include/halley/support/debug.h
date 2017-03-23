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

namespace Halley {
	struct DebugTraceEntry
	{
		const char* filename = nullptr;
		int line = 0;
		std::array<char, 1024> arg = {};
	};

	class Debug {
	public:
		static bool isDebug();

		static void setErrorHandling();
		static void printCallStack();
		static String getCallStack();

		static void trace(const char* filename, int line, const char* arg = nullptr);
		static String getLastTraces();

	private:
		Debug();
		static bool debugging;
		static std::array<DebugTraceEntry, 32> lastTraces;
		static int tracePos;
	};

	#define HALLEY_DEBUG_TRACE() Halley::Debug::trace(__FILE__, __LINE__)
	#define HALLEY_DEBUG_TRACE_COMMENT(str) Halley::Debug::trace(__FILE__, __LINE__, (str))
	#define HALLEY_DEBUG_TRACE_THIS() Halley::Debug::trace(__FILE__, __LINE__, typeid(*this).name())
}
