/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include "halley/os/os.h"
#include "os_win32.h"
#include "os_mac.h"
#include "os_android.h"
#include "os_ios.h"
#include "os_linux.h"
#include "os_freebsd.h"
#include "halley/support/exception.h"
#include <fstream>

using namespace Halley;

OS& Halley::OS::get()
{
	return *osInstance;
}

void OS::setInstance(OS* os)
{
	osInstance = os;
}

OS* OS::createOS()
{
#if defined(_WIN32) && !defined(WINDOWS_STORE)
	return new OSWin32();
#elif defined(__APPLE__)
	return new OSMac();
#elif defined(__ANDROID__)
	return new OSAndroid();
#elif defined(__IPHONEOS__)
	return new OSiOS();
#elif defined(linux)
	return new OSLinux();
#elif defined(__FreeBSD__) && !defined(__ORBIS__)
	return new OSFreeBSD();
#else
	return new OS();
#endif
}

void Halley::OS::createLogConsole(String /*name*/)
{
}

void OS::initializeConsole()
{
}

void OS::displayError(const std::string& cs)
{
}

void OS::onWindowCreated(void* windowHandle)
{
}

Halley::ComputerData Halley::OS::getComputerData()
{
	return ComputerData();
}

String OS::getUserDataDir()
{
	return "";
}

String OS::getCurrentWorkingDir()
{
	return "";
}

String OS::getEnvironmentVariable(const String&)
{
	return "";
}

Halley::String Halley::OS::makeDataPath(String appDataPath, String userProvidedPath)
{
	return appDataPath + userProvidedPath;
}

Path OS::parseProgramPath(const String& commandLine)
{
	return Path(commandLine).parentPath() / ".";
}

void OS::createDirectories(const Path& path)
{
}

void OS::atomicWriteFile(const Path& path, const Bytes& data, Maybe<Path> backupOldVersionPath)
{
	std::ofstream fp(path.string(), std::ios::binary | std::ios::out);
	fp.write(reinterpret_cast<const char*>(data.data()), data.size());
	fp.close();
}

std::vector<Path> OS::enumerateDirectory(const Path& path)
{
	return {};
}

void Halley::OS::setConsoleColor(int, int)
{
}

int OS::runCommand(String)
{
	throw Exception("Running commands is not implemented in this platform.", HalleyExceptions::OS);
}

std::shared_ptr<IClipboard> OS::getClipboard()
{
	return {};
}

void OS::openURL(const String& url)
{
}

OS* OS::osInstance = nullptr;
