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

#ifdef __FreeBSD__

#include "os_freebsd.h"
#include <sys/types.h>
#include <sys/sysctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
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

ComputerData OSFreeBSD::getComputerData()
{
	ComputerData data;

	{
		char buffer[128];
		buffer[127] = 0;
		gethostname(buffer, 127);
		data.computerName = buffer;
	}

	{
		size_t len = sizeof(data.RAM);
		sysctlbyname("hw.physmem", &data.RAM, &len, NULL, 0);
	}

	{
		char buffer[128] = {0};
		size_t len = sizeof(buffer);
		sysctlbyname("hw.model", buffer, &len, NULL, 0);
		data.cpuName = buffer;
	}

	{
		char buffer[128] = {0};
		size_t len = sizeof(buffer);
		sysctlbyname("kern.version", buffer, &len, NULL, 0);
		data.osName = String(buffer).trimBoth();
	}

	data.gpuName = slice(execCommand("pciconf -lv vgapci0"), "device     = '", "'\n").trimBoth();

	return data;
}

Path OSFreeBSD::parseProgramPath(const String& path)
{
	size_t len = 1024;
	char buffer[len];
	bzero(buffer, len);

	int mib[4] = {
		CTL_KERN,
		KERN_PROC,
		KERN_PROC_PATHNAME,
		-1,
	};

	sysctl(mib, 4, buffer, &len, NULL, 0);
	return Path(String(buffer)).parentPath() / ".";
}

String OSFreeBSD::getUserDataDir()
{
	String result;

	if (!(result = getenv("XDG_DATA_HOME")).isEmpty()) {
		return result;
	} else if ((result = getenv("HOME")).isEmpty()) {
		struct passwd* pwd = getpwuid(getuid());
		if ((result = pwd->pw_dir).isEmpty()) {
			throw Exception("Unable to find path to user data directory.", HalleyExceptions::OS);
		}
	}

	return result + "/.local/share";
}

void OSFreeBSD::openURL(const String& url)
{
	if (url.startsWith("http://") || url.startsWith("https://")) {
		auto str = "xdg-open " + url;
		system(str.c_str());
	}
}

#endif
