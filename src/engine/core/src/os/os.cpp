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
#include "os_gdk.h"
#include "halley/support/exception.h"
#include <fstream>

using namespace Halley;

OS& Halley::OS::get()
{
	HalleyAssertDev(osInstance != nullptr);
	return *osInstance;
}

void OS::setInstance(OS* os)
{
	osInstance = os;
}

OS* OS::createOS()
{
#if defined(_WIN32) && !defined(WITH_GDK)
	return new OSWin32();
#elif defined(__APPLE__)
	return new OSMac();
#elif defined(__ANDROID__)
	return new OSAndroid();
#elif defined(__IPHONEOS__)
	return new OSiOS();
#elif defined(linux)
	return new OSLinux();
#elif defined(__FreeBSD__) && !defined(__ORBIS__) && !defined(__PROSPERO__)
	return new OSFreeBSD();
#elif defined(WITH_GDK)
    return new OSGDK();
#else
	return new OS();
#endif
}

void OS::createLogConsole(String /*name*/, std::optional<size_t> monitor, Vector2f align)
{
}

void OS::initializeConsole()
{
}

void OS::displayError(std::string_view cs)
{
}

void OS::onWindowCreated(void* windowHandle)
{
}

Halley::ComputerData Halley::OS::getComputerData()
{
	return ComputerData();
}

String OS::getComputerName()
{
	return "Unknown";
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
	return Path(commandLine);
}

void OS::createDirectories(const Path& path)
{
}

bool OS::atomicWriteFile(const Path& path, gsl::span<const std::byte> data, std::optional<Path> backupOldVersionPath)
{
	std::ofstream fp(path.string(), std::ios::binary | std::ios::out);
	if (fp.is_open()) {
		fp.write(reinterpret_cast<const char*>(data.data()), data.size());
		fp.close();
		return true;
	}
	return false;
}

Vector<Path> OS::enumerateDirectory(const Path& path)
{
	return {};
}

void OS::setConsoleColor(int, int)
{
}

int OS::runCommand(String, String, ILoggerSink* sink)
{
	throw Exception("Running commands is not implemented in this platform.", HalleyExceptions::OS);
}

Future<int> OS::runCommandAsync(const String& string, const String& cwd, ILoggerSink* sink)
{
	throw Exception("Running commands is not implemented in this platform.", HalleyExceptions::OS);
}

bool OS::runCommandDetached(const String& string, const String& cwd)
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

void OS::openFile(const Path& path)
{
}

void OS::showFile(const Path& path)
{
}

Future<std::optional<Path>> OS::openFileChooser(FileChooserParameters)
{
	return Future<std::optional<Path>>::makeImmediate({});
}

uint64_t OS::getMemoryUsage()
{
	return 0;
}

bool OS::isDebuggerAttached() const
{
    return false;
}

String OS::runQuery(std::string_view query, const String& parameter, std::string_view queryNamespace) const
{
	auto result = runQuery(query, gsl::span<const String>(&parameter, 1), queryNamespace);
	if (result.empty()) {
		return "";
	}
	return result[0];
}

Vector<String, std::allocator<String>, 0, true> OS::runQuery(std::string_view query, gsl::span<const String> parameters, std::string_view queryNamespace) const
{
	return {};
}

ConfigNode OS::getRegistryString(std::string_view key) const
{
	return {};
}

void OS::setEnvVariable(std::string_view name, std::string_view value)
{
}

OS* OS::osInstance = nullptr;
