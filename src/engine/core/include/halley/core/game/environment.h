#pragma once

#include <halley/text/halleystring.h>
#include "halley/file/path.h"

namespace Halley
{
	class Environment
	{
	public:
		Path getProgramPath() const;
		Path getDataPath() const;
		Path getGameDataPath() const;

		void parseProgramPath(const String& commandLine);
		void setDataPath(Path pathName);

	private:
		Path programPath;
		Path dataPath;
		Path gameDataPath;
	};
}
