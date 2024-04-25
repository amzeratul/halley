#pragma once

#include <halley/text/halleystring.h>
#include "halley/file/path.h"

namespace Halley
{
	class Environment
	{
	public:
		Path getProgramPath() const;
		Path getProgramExecutablePath() const;
		Path getDataPath() const;
		Path getGameDataPath() const;
		const Vector<std::string>& getArguments() const;
		bool hasArgument(const std::string& arg) const;

		void parseProgramPath(const String& commandLine);
		void setDataPath(Path pathName);
		void setArguments(const Vector<std::string>& args);

	private:
		Path programPath;
		Path programExecutablePath;
		Path dataPath;
		Path gameDataPath;
		Vector<std::string> args;
	};
}
