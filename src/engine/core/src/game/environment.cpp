#include <halley/os/os.h>
#include "halley/core/game/environment.h"
#include "halley/file/path.h"
#include "halley/utils/algorithm.h"

Halley::Path Halley::Environment::getProgramPath() const
{
	return programPath;
}

Halley::Path Halley::Environment::getDataPath() const
{
	return dataPath;
}

Halley::Path Halley::Environment::getGameDataPath() const
{
	return gameDataPath;
}

const Halley::Vector<std::string>& Halley::Environment::getArguments() const
{
	return args;
}

bool Halley::Environment::hasArgument(const std::string& arg) const
{
	return std_ex::contains(args, arg);
}

void Halley::Environment::parseProgramPath(const String& commandLine)
{
	programPath = OS::get().parseProgramPath(commandLine);

#ifdef __ANDROID__
	gameDataPath = Path(); // Inside "assets"
#else
	gameDataPath = programPath;
#endif
}

void Halley::Environment::setDataPath(Path pathName)
{
	dataPath = Path(OS::get().getUserDataDir()) / pathName / ".";
	OS::get().createDirectories(dataPath);
}

void Halley::Environment::setArguments(const Vector<std::string>& args)
{
	this->args = args;
}
