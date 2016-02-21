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

#include "../text/halleystring.h"
#include <list>

namespace Halley {
	class Debug {
	public:
		static void setDebugMode(bool enabled);
		static void toggleDebugMode();
		static bool isDebugMode();

		static void setErrorHandling();
		static void printCallStack();
		static String getCallStack();

		static void trace(String str);
		static String getLastTraces();

	private:
		Debug();
		static bool debugging;
		static std::list<String> lastTraces;
	};
}
