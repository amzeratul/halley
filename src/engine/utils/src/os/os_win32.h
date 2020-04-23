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

#if defined(_WIN32) && !defined(WINDOWS_STORE)
#include <halley/os/os.h>
#define _WIN32_DCOM
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wbemidl.h>

namespace Halley {
	class OSWin32 final : public OS {
	public:
		OSWin32();
		~OSWin32();

		void createLogConsole(String name) override;
		void initializeConsole() override;

		ComputerData getComputerData() override;
		String getUserDataDir() override;
		String getCurrentWorkingDir() override;
		String getEnvironmentVariable(const String& name) override;
		Path parseProgramPath(const String&) override;
		void setConsoleColor(int foreground, int background) override;
		void createDirectories(const Path& path) override;
		void atomicWriteFile(const Path& path, const Bytes& data, std::optional<Path> backupOldVersionPath) override;
		std::vector<Path> enumerateDirectory(const Path& path) override;

		void displayError(const std::string& cs) override;
		void onWindowCreated(void* window) override;

		int runCommand(String command) override;
		Future<int> runCommandAsync(const String& string) override;
		void runCommand(StringUTF16 command, Promise<int> promise);

		std::shared_ptr<IClipboard> getClipboard() override;

		void openURL(const String& url) override;

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
