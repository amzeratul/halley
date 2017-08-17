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
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wbemidl.h>

namespace Halley {
	class OSWin32 : public OS {
	public:
		OSWin32();
		~OSWin32();

		void createLogConsole(String name) override;
		void initializeConsole() override;

		ComputerData getComputerData() override;
		String getUserDataDir() override;
		Path parseProgramPath(const String&) override;
		void setConsoleColor(int foreground, int background) override;
		void createDirectories(const Path& path) override;
		std::vector<Path> enumerateDirectory(const Path& path) override;

		void onWindowCreated(void* window) override;

		int runCommand(String command) override;

	private:
		String runWMIQuery(String query, String parameter) const;
		void loadWindowIcon(HWND hwnd);

	private:
		IWbemLocator *pLoc;
		IWbemServices *pSvc;
		HICON icon;
	};
}

#endif
