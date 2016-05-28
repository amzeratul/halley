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

namespace Halley {
	class ComputerData {
	public:
		String computerName;
		String userName;
		String cpuName;
		String gpuName;
		String osName;
		long long RAM = 0;
	};

	class OS {
	public:
		virtual ~OS() {}
		static OS& get();
		static void setInstance(OS* os);
		static OS* createOS();

		virtual void createLogConsole(String name);

		virtual ComputerData getComputerData();
		virtual String getUserDataDir()=0;
		virtual String makeDataPath(String appDataPath, String userProvidedPath);
		virtual void setConsoleColor(int foreground, int background);

	private:
		static OS* osInstance;
	};
}
