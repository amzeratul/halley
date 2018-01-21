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

  Copyright (c) 2007-2017 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#ifdef linux

#include "os_linux.h"
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <cstring>

using namespace Halley;


String execCommand(String command) {
	FILE* fp = popen(command.c_str(), "r");
	if (!fp) {
		return "";
	}
	String result;
	constexpr int bufSize = 1024;
	char buffer[bufSize];
	while (fgets(buffer, bufSize - 1, fp)) {
		result += buffer;
	}
	pclose(fp);
	return result;
}

String slice(String str, const char* from, const char* to) {
	String low = str.asciiLower();
	size_t pos = low.find(from);
	if (pos == std::string::npos) {
		return "";
	}
	pos += strlen(from);
	size_t end = low.find(to, pos + 1);
	return str.substr(pos, end - pos);
}

ComputerData OSLinux::getComputerData()
{
	ComputerData data;

	{
		char buffer[128];
		buffer[127] = 0;
		gethostname(buffer, 127);
		data.computerName = buffer;
	}

	{
		struct sysinfo info;
		sysinfo(&info);
		data.RAM = info.totalram;
	}

	data.gpuName = slice(execCommand("lspci"), "vga compatible controller: ", "\n").trimBoth();
	data.cpuName = slice(execCommand("lscpu"), "model name: ", "\n").trimBoth();
	data.osName = execCommand("uname -a").trimBoth();

	return data;
}

Path OSLinux::parseProgramPath(const String& path)
{
	constexpr size_t len = 1024;
	char buffer[len];
	if (readlink("/proc/self/exe", buffer, len) != -1) {
		return Path(String(buffer)).parentPath() / ".";
	} else {
		return OSUnix::parseProgramPath(path);
	}
}

String OSLinux::getUserDataDir()
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