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
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>

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

int Halley::OSUnix::runCommand(String command)
{
	auto args = command.split(' ');
	String cmd = args[0];
	std::vector<const char*> argsPtr(args.size());
	for (size_t i = 1; i < args.size(); ++i) {
		argsPtr[i - 1] = args[i].c_str();
	}
	argsPtr[args.size() - 1] = nullptr;

	auto pid = fork();
	if (pid == -1) {
		throw Exception("Unable to fork process.");
	} else if (pid == 0) {
		// Child
		execvp(cmd.c_str(), argsPtr.data());
		
		return -1; // Will never get reached, but whatever
	} else {
		int status;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status)) {
			return WEXITSTATUS(status);
		} else {
			return -1;
		}
	}
}

#endif
