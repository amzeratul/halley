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

#include "halley/support/debug.h"
#include "halley/text/halleystring.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include "halley/os/os.h"
#include "halley/text/string_converter.h"

#if defined(_MSC_VER) && !defined(WINDOWS_STORE)
#define HAS_STACKTRACE
#include <boost/stacktrace.hpp>
#endif

#if !defined(__NX_TOOLCHAIN_MAJOR__) && !defined(__ORBIS__)
#define HAS_SIGNAL
#include <csignal>
#endif


using namespace Halley;

Debug::Debug()
{
}

static String dumpFile;
static std::function<void(const std::string&)> errorHandler;


#ifdef HAS_SIGNAL
static void signalHandler(int signum)
{
    ::signal(SIGSEGV, SIG_DFL);
	::signal(SIGABRT, SIG_DFL);

#ifdef HAS_STACKTRACE
	boost::stacktrace::safe_dump_to(dumpFile.c_str());
#endif

	std::stringstream ss;
	ss << "Process aborting with signal #";
	switch (signum) {
	case SIGINT:
		ss << "SIGINT (2)";
		break;
	case SIGILL:
		ss << "SIGILL (4)";
		break;
	case SIGFPE:
		ss << "SIGFPE (8)";
		break;
	case SIGSEGV:
		ss << "SIGSEGV (11)";
		break;
	case SIGTERM:
		ss << "SIGTERM (15)";
		break;
	case SIGABRT:
		ss << "SIGABRT (22)";
		break;
	default:
		ss << "UNKNOWN (" << signum << ")";
	}

#ifdef HAS_STACKTRACE
	ss << "\n" << boost::stacktrace::stacktrace(3, 99);
#endif
	errorHandler(ss.str());

	::raise(SIGABRT);
}
#endif

static void terminateHandler()
{
	std::stringstream ss;
	ss << "std::terminate() invoked.";
#ifdef HAS_STACKTRACE
	ss << "\n" << boost::stacktrace::stacktrace(3, 99);
#endif
	errorHandler(ss.str());

	std::abort();
}


void Debug::setErrorHandling(const String& dumpFilePath, std::function<void(const std::string&)> eh)
{
	dumpFile = dumpFilePath;

#ifdef HAS_SIGNAL
	::signal(SIGSEGV, &signalHandler);
	::signal(SIGABRT, &signalHandler);
#endif

	std::set_terminate(&terminateHandler);
	errorHandler = eh;
}


String Debug::getCallStack(int skip)
{
	std::stringstream ss;
#ifdef HAS_STACKTRACE
	ss << boost::stacktrace::stacktrace(skip, 99);
#endif
	return ss.str();
}

#ifdef min
#undef min
#endif

void Debug::trace(const char* filename, int line, const char* arg)
{
	auto& trace = lastTraces[tracePos];
	tracePos = (tracePos + 1) % lastTraces.size();
	trace.filename = filename;
	trace.line = line;

	if (arg) {
		size_t len = std::min(trace.arg.size() - 1, strlen(arg));
		memcpy(trace.arg.data(), arg, len);
		trace.arg[len] = 0;
	} else {
		trace.arg[0] = 0;
	}
}

String Debug::getLastTraces()
{
	String result;
	const size_t n = lastTraces.size();
	for (size_t i = 0; i < n; ++i) {
		auto& trace = lastTraces[(i + tracePos) % n];
		result += " - " + String(trace.filename) + ":" + toString(trace.line);
		if (trace.arg[0] != 0) {
			result += String(" [") + trace.arg.data() + "]";
		}
		if (i == n - 1) {
			result += " [latest]";
		}
		result += "\n";
	}
	return result;
}

void Debug::printLastTraces()
{
	const size_t n = lastTraces.size();
	for (size_t i = 0; i < n; ++i) {
		auto& trace = lastTraces[(i + tracePos) % n];
		if (!trace.filename) {
			break;
		}
		std::cout << " - " << trace.filename << ":" << toString(trace.line);
		if (trace.arg[0] != 0) {
			std::cout << " [" << trace.arg.data() << "]";
		}
		if (i == n - 1) {
			std::cout << " [latest]";
		}
		std::cout << std::endl;
	}
}

std::array<DebugTraceEntry, 32> Debug::lastTraces;
int Debug::tracePos = 0;
