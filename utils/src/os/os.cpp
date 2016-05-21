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

#include "../../include/halley/os/os.h"
#include "os_win32.h"
#include "os_mac.h"
#include "os_android.h"
#include "os_ios.h"
#include "os_linux.h"
#include "../../include/halley/support/exception.h"

using namespace Halley;

OS& Halley::OS::get()
{
	static OS* instance = nullptr;
	if (!instance) {
#if defined(_WIN32)
		instance = new OSWin32();
#elif defined(__APPLE__)
		instance = new OSMac();
#elif defined(__ANDROID__)
		instance = new OSAndroid();
#elif defined(__IPHONEOS__)
		instance = new OSiOS();
#elif defined(linux)
        instance = new OSLinux();
#else
		throw Exception("Unknown OS - please update os.cpp on Halley.");
#endif
	}
	return *instance;
}

void Halley::OS::createLogConsole(String /*name*/)
{
}

Halley::ComputerData Halley::OS::getComputerData()
{
	return ComputerData();
}

Halley::String Halley::OS::makeDataPath(String appDataPath, String userProvidedPath)
{
	return appDataPath + userProvidedPath;
}

void Halley::OS::setConsoleColor(int, int)
{
}
