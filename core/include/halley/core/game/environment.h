#pragma once

#include <halley/text/halleystring.h>

namespace Halley
{
	class Environment
	{
		friend class Core;

	public:
		String getProgramPath() const { return programPath;	}
		String getDataPath() const { return dataPath; }
		String getGameDataPath() const { return gameDataPath; }

	private:
		void parseProgramPath(String path);
		void setDataPath(String pathName);

		String programPath;
		String dataPath;
		String gameDataPath;
	};
}
