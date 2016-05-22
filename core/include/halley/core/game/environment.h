#pragma once

#include <halley/text/halleystring.h>

namespace Halley
{
	class Environment
	{
		friend class CoreRunner;

	public:
		static String getProgramPath() { return getInstance()->programPath;	}
		static String getDataPath() { return getInstance()->dataPath; }
		static String getGameDataPath() { return getInstance()->gameDataPath; }

	private:
		static void parseProgramPath(String path);
		static void setDataPath(String pathName);

		static Environment* getInstance();
		String programPath;
		String dataPath;
		String gameDataPath;
	};
}
