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
#include <dirent.h>
#include <fcntl.h>
#include <sys/poll.h>
#include "halley/utils/halley_iostream.h"

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

String Halley::OSUnix::getCurrentWorkingDir()
{
	String result;
	result.setSize(PATH_MAX);

	char* cwd;
	do {
		cwd = getcwd((char*) result.c_str(), result.size());
		if (cwd == nullptr) {
			result.setSize(result.size() + PATH_MAX);
		}
	} while (cwd == nullptr);

	result.truncate(strlen(cwd));

	return result;
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

static String readPipeToString(int fd)
{
	char buffer[4096];
	String result;

	struct pollfd pfd = {};
	pfd.fd = fd;
	pfd.events = POLLIN;

	size_t bytesRead;

	// The Windows version uses a 10ms timeout during WaitForMultipleObjects().
	// waitpid() doesn't support that, so we wait on the poll() call instead.

	while (poll(&pfd, 1, 10) == 1) {
		bytesRead = read(fd, buffer, sizeof(buffer) - 1);
		buffer[bytesRead] = 0;
		result += buffer;
		if (((pfd.revents & POLLHUP) != 0) && (bytesRead == 0)) {
			break;
		}
	}

	return result;
}

int Halley::OSUnix::runCommand(String command, String cwd, ILoggerSink* sink)
{
	Promise<int> promise;
	auto future = promise.getFuture();

	runCommand(command, cwd, promise, sink);

	return future.get();
}

Future<int> OSUnix::runCommandAsync(const String& command, const String& cwd, ILoggerSink* sink)
{
	Promise<int> promise;
	auto future = promise.getFuture();

	std::thread([this, command, cwd, promise = std::move(promise), sink] () {
		runCommand(command, cwd, promise, sink);
	}).detach();

	return future;
}

void Halley::OSUnix::runCommand(String command, String cwd, Promise<int> promise, ILoggerSink* sink)
{
	auto args = command.split(' ');
	Vector<char*> argsPtr(args.size() + 1);
	for (size_t i = 0; i < args.size(); ++i) {
		argsPtr[i] = const_cast<char*>(args[i].c_str());
	}
	argsPtr[args.size()] = nullptr;

	int fd[2];
	pid_t child;

	if (pipe(fd) != 0) {
		throw Exception("Pipe error.", HalleyExceptions::OS);
	}

	child = fork();

	if (child == -1) {
		throw Exception("Unable to fork process.", HalleyExceptions::OS);
	} else if (child == 0) {
		dup2(fd[1], STDOUT_FILENO);
		dup2(fd[1], STDERR_FILENO);

		close(fd[0]);
		close(fd[1]);

		execvp(argsPtr[0], argsPtr.data());

		exit(1);
	}

	close(fd[1]);

	String outBuffer;
	auto readOutput = [&] (bool isFinal)
	{
		outBuffer += readPipeToString(fd[0]);

		for (auto lineBreakPos = outBuffer.find('\n'); lineBreakPos != std::string::npos; lineBreakPos = outBuffer.find('\n')) {
			Logger::logTo(sink, LoggerLevel::Info, outBuffer.left(lineBreakPos));
			outBuffer = outBuffer.mid(lineBreakPos + 1);
		}

		if (isFinal && !outBuffer.isEmpty()) {
			Logger::logTo(sink, LoggerLevel::Info, outBuffer);
		}
	};

	pid_t wait;
	int status = 0;

	do {
		wait = waitpid(child, &status, WNOHANG);
		readOutput(false);
	} while (wait == 0);

	readOutput(true);

	promise.setValue((wait >= 0 && WIFEXITED(status)) ? WEXITSTATUS(status) : 1);

	close(fd[0]);
}

// Code by Jonathan Leffler, from http://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux/675193#675193
typedef struct stat Stat;
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

Vector<Path> Halley::OSUnix::enumerateDirectory(const Path& path)
{
	std::list<String> dirsToList;
	dirsToList.emplace_back(".");

	Vector <Path> result;

	DIR *d;
	struct dirent *dir;
	struct stat s = {};

	while (!dirsToList.empty()) {
		const auto curDir = dirsToList.front();
		const auto curPath = (path / curDir).getString(false);
		dirsToList.pop_front();

		d = opendir(curPath.c_str());
		if (d) {
			while ((dir = readdir(d)) != nullptr) {
				String curFile = String(dir->d_name);
				int err = stat((path / curDir / curFile).getString().c_str(), &s);
				if (err == 0) {
					if (S_ISDIR(s.st_mode)) {
						if (curFile != "." && curFile != "..") {
							dirsToList.emplace_back(curDir + "/" + curFile);
						}
					} else {
						auto res = Path(curDir) / String(dir->d_name);
						result.emplace_back(res);
					}
				}
			}

			closedir(d);
		}
	}

	return result;
}

#endif
