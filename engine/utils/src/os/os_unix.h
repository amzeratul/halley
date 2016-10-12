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


#if defined(__APPLE__) || defined(__ANDROID__) || defined(linux)
#define IS_UNIX

#include <halley/os/os.h>

namespace Halley {
	class OSUnix : public OS {
	public:
		OSUnix();
		~OSUnix();

		virtual ComputerData getComputerData();
		virtual String getUserDataDir();

		int runCommand(String command) override;
	};
}

#endif
