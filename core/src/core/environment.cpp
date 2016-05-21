#include <halley/os/os.h>
#include "core/environment.h"

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <unistd.h>
#endif

void Halley::Environment::parseProgramPath(String path)
{
	auto instance = getInstance();

#ifdef __APPLE__
	// Ignore parameter and use our own path
	char buffer[2048];
	uint32_t bufSize = 2048;
	_NSGetExecutablePath(buffer, &bufSize);
	path = buffer;
#endif

	size_t len = path.length();
	size_t last = 0;
	for (size_t i = 0; i<len; i++) {
		if (path[i] == '/' || path[i] == '\\') last = i + 1;
	}
	instance->programPath = path.left(last);

#ifdef __APPLE__
	std::cout << "Setting CWD to " << programPath << std::endl;
	chdir(programPath.c_str());
#endif	

#ifdef __ANDROID__
	instance->gameDataPath = ""; // Inside "assets"
#else
	instance->gameDataPath = instance->programPath;
#endif
}

void Halley::Environment::setDataPath(String pathName)
{
	auto& os = OS::get();
	getInstance()->dataPath = os.makeDataPath(os.getUserDataDir(), pathName);
}

Halley::Environment* Halley::Environment::getInstance()
{
	static Environment* instance = nullptr;
	if (!instance) {
		instance = new Environment();
	}
	return instance;
}
