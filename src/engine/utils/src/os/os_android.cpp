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

#ifdef __ANDROID__

#include "os_android.h"
#include <stdlib.h>
#include <stdio.h>    
#include <pwd.h>
#include <unistd.h>
#include <android/log.h>

using namespace Halley;

Halley::OSAndroid::OSAndroid()
{
	__android_log_write(ANDROID_LOG_VERBOSE, "Halley", "Trace: OSAndroid::OSAndroid");
}

Halley::String Halley::OSAndroid::getUserDataDir()
{
	return "";
}

Halley::String Halley::OSAndroid::makeDataPath(String, String)
{
	return "";
}

#endif
