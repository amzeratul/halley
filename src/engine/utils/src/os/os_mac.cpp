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

#ifdef __APPLE__

#include "os_mac.h"
#include <stdlib.h>
#include <stdio.h>    
#include <pwd.h>
#include <unistd.h>
#include <iostream>
#include <mach-o/dyld.h>
#include "halley/utils/halley_iostream.h"

using namespace Halley;

String OSMac::getUserDataDir()
{
	String result;
	struct passwd* pwd = getpwuid(getuid());
	if (pwd) {
		result = pwd->pw_dir;
	} else {
		result = getenv("HOME");
	}

	return result + "/Library/";
}

Path OSMac::parseProgramPath(const String&)
{
	// Ignore parameter and use our own path
	char buffer[2048];
	uint32_t bufSize = 2048;
	_NSGetExecutablePath(buffer, &bufSize);
	Path programPath = Path(String(buffer)).parentPath() / ".";

	std::cout << "Setting CWD to " << programPath << std::endl;
	chdir(programPath.string().c_str());

	return programPath;
}

void OSMac::openURL(const String& url)
{
	if (url.startsWith("http://") || url.startsWith("https://")) {
		auto str = "open " + url;
		system(str.c_str());
	}
}

#endif