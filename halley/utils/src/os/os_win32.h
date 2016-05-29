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

#ifdef _WIN32
#include <halley/os/os.h>
#define _WIN32_DCOM
#include <Windows.h>
#include <wbemidl.h>

namespace Halley {
	class OSWin32 : public OS {
	public:
		OSWin32();
		~OSWin32();

		virtual void createLogConsole(String name);

		virtual ComputerData getComputerData();
		virtual String getUserDataDir();
		void setConsoleColor(int foreground, int background);

	private:
		String runWMIQuery(String query, String parameter);

		IWbemLocator *pLoc;
		IWbemServices *pSvc;
	};
}

#endif
