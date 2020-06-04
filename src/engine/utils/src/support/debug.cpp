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
#define HAS_STACKWALKER
#include "StackWalker/StackWalker.h"

class OStreamStackWalker : public StackWalker {
public:
	OStreamStackWalker(std::ostream& os, int startFrom)
		: os(os)
		, startFrom(startFrom)
	{}

	OStreamStackWalker(std::ostream& os, const char* skipUntil, int offset)
		: os(os)
		, skipUntil(skipUntil)
		, offset(offset)
	{}

protected:
	void OnCallstackEntry(CallstackEntryType eType, CallstackEntry& entry) override
	{
		if (eType == firstEntry) {
			curPos = 0;
		}

		if (skipUntil && !foundSkip) {
			if (std::strstr(entry.name, skipUntil) == nullptr) {
				startFrom = curPos + offset + 2;
			} else {
				foundSkip = true;
			}
		}

		if (++curPos < startFrom) {
			return;
		}

		os << ' ' << (curPos - startFrom) << ": " << entry.name;
		if (entry.lineFileName[0] != 0) {
			const char* lastSlash = strrchr(entry.lineFileName, '\\');
			if (lastSlash) {
				++lastSlash;
			} else {
				lastSlash = entry.lineFileName;
			}
			os << " at " << lastSlash << ':' << entry.lineNumber;
		} else if (entry.moduleName[0] != 0) {
			os << " [" << entry.moduleName << ']';
		}
		os << '\n';
	}
	
private:
	std::ostream& os;
	int startFrom = 0;
	int curPos = 0;
	int offset = 0;
	const char* skipUntil = nullptr;
	bool foundSkip = false;
};

#endif

#if false
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

#if defined(HAS_STACKWALKER)
	ss << "\n";
	OStreamStackWalker walker(ss, "RtlRaiseException", 2);
	walker.ShowCallstack();
#elif defined(HAS_STACKTRACE)
	ss << "\n" << boost::stacktrace::stacktrace(3, 99);
#endif
	errorHandler(ss.str());

	::raise(SIGABRT);
}
#endif

static void terminateHandler()
{
	std::stringstream ss;
	ss << "std::terminate() invoked.\n";

#if defined(HAS_STACKWALKER)
	OStreamStackWalker walker(ss, 3);
	walker.ShowCallstack();
#elif defined(HAS_STACKTRACE)
	ss << boost::stacktrace::stacktrace(3, 99);
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
	errorHandler = std::move(eh);
}


String Debug::getCallStack(int skip)
{
	std::stringstream ss;
#if defined(HAS_STACKWALKER)
	OStreamStackWalker walker(ss, skip);
	walker.ShowCallstack();
#elif defined(HAS_STACKTRACE)
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
	tracePos = (tracePos + 1) % int(lastTraces.size());
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

std::array<DebugTraceEntry, 16> Debug::lastTraces;
int Debug::tracePos = 0;
