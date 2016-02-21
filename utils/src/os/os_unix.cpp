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

#include "os_unix.h"
#ifdef IS_UNIX

#include <stdlib.h>
#include <stdio.h>    
#include <pwd.h>
#include <unistd.h>

using namespace Halley;


Halley::OSUnix::OSUnix()
{
}

Halley::OSUnix::~OSUnix()
{
}

Halley::ComputerData Halley::OSUnix::getComputerData()
{
	ComputerData data;
	// TODO
	return data;
}

Halley::String Halley::OSUnix::getUserDataDir()
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

#endif
