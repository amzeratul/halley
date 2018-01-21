#pragma once

#include <halley/text/halleystring.h>
#include "halley/file/path.h"

namespace Halley
{
	class Environment
	{
		friend class Core;

	public:
		Path getProgramPath() const;
		Path getDataPath() const;
		Path getGameDataPath() const;

	private:
		void parseProgramPath(const String& commandLine);
		void setDataPath(Path pathName);

		Path programPath;
		Path dataPath;
		Path gameDataPath;
	};
}
