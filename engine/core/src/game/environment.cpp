#include <halley/os/os.h>
#include "halley/core/game/environment.h"
#include "halley/file/path.h"

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
