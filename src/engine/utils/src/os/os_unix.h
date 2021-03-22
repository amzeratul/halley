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


#if defined(__APPLE__) || defined(__ANDROID__) || defined(linux) || (defined(__FreeBSD__) && !defined(__ORBIS__))
#define IS_UNIX

#include <halley/os/os.h>

namespace Halley {
	class OSUnix : public OS {
	public:
		OSUnix();
		~OSUnix();

		virtual ComputerData getComputerData() override;
		virtual String getUserDataDir() override;
		void createDirectories(const Path& path) override;
		std::vector<Path> enumerateDirectory(const Path& path) override;

		int runCommand(String command, String cwd, ILoggerSink* sink) override;
	};
}

#endif
