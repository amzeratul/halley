#include <halley/os/os.h>
#include "halley/core/game/environment.h"
#include "halley/file/path.h"

#ifdef __APPLE__
#include <iostream>
#include <mach-o/dyld.h>
#include <unistd.h>
#endif

void Halley::Environment::parseProgramPath(String path)
{
#ifdef __APPLE__
	// Ignore parameter and use our own path
	char buffer[2048];
	uint32_t bufSize = 2048;
	_NSGetExecutablePath(buffer, &bufSize);
	path = buffer;
#endif

	// HACK
	size_t len = path.length();
	size_t last = 0;
	for (size_t i = 0; i<len; i++) {
		if (path[i] == '/' || path[i] == '\\') last = i + 1;
	}
	programPath = path.left(last);

#ifdef __APPLE__
	std::cout << "Setting CWD to " << programPath << std::endl;
	chdir(programPath.c_str());
#endif	

#ifdef __ANDROID__
	gameDataPath = ""; // Inside "assets"
#else
	gameDataPath = programPath;
#endif
}

void Halley::Environment::setDataPath(String pathName)
{
	auto& os = OS::get();

	String p = (Path(os.getUserDataDir()) / pathName / ".").getString();
	//FileSystem::createDir(p);

	dataPath = p;
}
