#include <halley/os/os.h>
#include "halley/game/environment.h"
#include "halley/file/path.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

Path Environment::getProgramPath() const
{
	return programPath;
}

Path Environment::getProgramExecutablePath() const
{
	return programExecutablePath;
}

Path Environment::getDataPath() const
{
	return dataPath;
}

Path Environment::getGameDataPath() const
{
	return gameDataPath;
}

const Vector<std::string>& Environment::getArguments() const
{
	return args;
}

bool Environment::hasArgument(const std::string& arg) const
{
	return std_ex::contains(args, arg);
}

void Environment::parseProgramPath(const String& commandLine)
{
	programExecutablePath = OS::get().parseProgramPath(commandLine);
	programPath = programExecutablePath.parentPath() / ".";

#ifdef __ANDROID__
	gameDataPath = Path(); // Inside "assets"
#else
	gameDataPath = programPath;
#endif
}

void Environment::setDataPath(Path pathName)
{
	dataPath = Path(OS::get().getUserDataDir()) / pathName / ".";
	OS::get().createDirectories(dataPath);
}

void Environment::setArguments(const Vector<std::string>& args)
{
	this->args = args;
}
