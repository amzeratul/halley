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

#include <halley/support/exception.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>

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
	std::vector<char*> argsPtr(args.size());
	for (size_t i = 1; i < args.size(); ++i) {
		argsPtr[i - 1] = const_cast<char*>(args[i].c_str());
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

// Code by Jonathan Leffler, from http://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux/675193#675193
static int do_mkdir(const char *path, mode_t mode)
{
    Stat            st;
    int             status = 0;

    if (stat(path, &st) != 0)
    {
        /* Directory does not exist. EEXIST for race condition */
        if (mkdir(path, mode) != 0 && errno != EEXIST)
            status = -1;
    }
    else if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        status = -1;
    }

    return(status);
}

/**
** mkpath - ensure all directories in path exist
** Algorithm takes the pessimistic view and works top-down to ensure
** each directory in path exists, rather than optimistically creating
** the last element and working backwards.
*/
int mkpath(const char *path, mode_t mode)
{
    char           *pp;
    char           *sp;
    int             status;
    char           *copypath = strdup(path);

    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = do_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0)
        status = do_mkdir(path, mode);
    free(copypath);
    return (status);
}

void Halley::OSUnix::createDirectories(const Path& path)
{
	mkpath(path.string().c_str(), 0777);
}

#endif
