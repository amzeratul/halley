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
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"

#if defined(_MSC_VER) && !defined(WITH_GDK)
#define HAS_STACKWALKER
#include "StackWalker/StackWalker.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

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


class NoAllocStackWalker : public StackWalker {
public:
	NoAllocStackWalker(gsl::span<char> dst, int startFrom)
		: dst(dst.data())
		, spaceLeft(dst.size() - strlen(dst.data()))
		, startFrom(startFrom)
	{}

	NoAllocStackWalker(gsl::span<char> dst, const char* skipUntil, int offset)
		: dst(dst.data())
		, spaceLeft(dst.size() - strlen(dst.data()))
		, offset(offset)
		, skipUntil(skipUntil)
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

		cat(" ");
		cat(curPos - startFrom);
		cat(": ");
		cat(entry.name);
		if (entry.lineFileName[0] != 0) {
			const char* lastSlash = strrchr(entry.lineFileName, '\\');
			if (lastSlash) {
				++lastSlash;
			} else {
				lastSlash = entry.lineFileName;
			}
			cat(" at ");
			cat(lastSlash);
			cat(":");
			cat(static_cast<int>(entry.lineNumber));
		} else if (entry.moduleName[0] != 0) {
			cat(" [");
			cat(entry.moduleName);
			cat("]");
		}
		cat("\n");
	}

	void cat(const char* str)
	{
		const auto len = strlen(str);
		if (spaceLeft >= len + 1) {
			strcpy(dst, str);
			spaceLeft -= len;
		} else {
			spaceLeft = 0;
		}
	}

	void cat(int value)
	{
		char buffer[32];
		_itoa(value, buffer, 10);
		cat(buffer);
	}
	
private:
	char* dst;
	size_t spaceLeft = 0;
	int startFrom = 0;
	int curPos = 0;
	int offset = 0;
	const char* skipUntil = nullptr;
	bool foundSkip = false;
};

#endif

#if !defined(NN_NINTENDO_SDK) && !defined(__ORBIS__)
#define HAS_SIGNAL
#include <csignal>
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

using namespace Halley;

Debug::Debug()
{
}

namespace {
	String dumpFile;
	std::function<void(std::string_view)> errorHandler;

#ifdef HAS_SIGNAL
	void signalHandler(int signum)
	{
	    ::signal(SIGSEGV, SIG_DFL);
		::signal(SIGABRT, SIG_DFL);

		const char* name = nullptr;

		switch (signum) {
		case SIGINT:
			name = "External Interrupt (SIGINT)";
			break;
		case SIGILL:
			name = "Invalid Program Image (SIGILL)";
			break;
		case SIGFPE:
			name = "Erroneous Arithmetic Operation (SIGFPE)";
			break;
		case SIGSEGV:
			name = "Segmentation Fault (SIGSEGV)";
			break;
		case SIGTERM:
			name = "Termination Request (SIGTERM)";
			break;
		case SIGABRT:
			name = "Abnormal Termination Condition (SIGABRT)";
			break;
		default:
			name = "Unknown";
		}

		std::cout << "Process aborting due to: " << name << " (" << signum << ")\n";
		std::cout << "[start of stack trace]\n";
		Debug::printCallStackTo(std::cout, 4);
		std::cout << "[end of stack trace]\n";

		if (errorHandler) {
			errorHandler(name);
		}

		Debug::abort();
	}
#endif

	void terminateHandler()
	{
		std::cout << "std::terminate() invoked.\n";
		std::cout << "[start of stack trace]\n";
		Debug::printCallStackTo(std::cout, 4);
		std::cout << "[end of stack trace]\n";

		errorHandler("std::terminate() invoked.");

		Debug::abort();
	}

#ifdef WIN32
	LONG WINAPI win32ExceptionHandler(EXCEPTION_POINTERS* exceptionInfo)
	{
		const char* name = nullptr;

		switch (exceptionInfo->ExceptionRecord->ExceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION:
			name = "EXCEPTION_ACCESS_VIOLATION";
			break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			name = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
			break;
		case EXCEPTION_BREAKPOINT:
			name = "EXCEPTION_BREAKPOINT";
			break;
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			name = "EXCEPTION_DATATYPE_MISALIGNMENT";
			break;
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			name = "EXCEPTION_FLT_DENORMAL_OPERAND";
			break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			name = "EXCEPTION_FLT_DIVIDE_BY_ZERO";
			break;
		case EXCEPTION_FLT_INEXACT_RESULT:
			name = "EXCEPTION_FLT_INEXACT_RESULT";
			break;
		case EXCEPTION_FLT_INVALID_OPERATION:
			name = "EXCEPTION_FLT_INVALID_OPERATION";
			break;
		case EXCEPTION_FLT_OVERFLOW:
			name = "EXCEPTION_FLT_OVERFLOW";
			break;
		case EXCEPTION_FLT_STACK_CHECK:
			name = "EXCEPTION_FLT_STACK_CHECK";
			break;
		case EXCEPTION_FLT_UNDERFLOW:
			name = "EXCEPTION_FLT_UNDERFLOW";
			break;
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			name = "EXCEPTION_ILLEGAL_INSTRUCTION";
			break;
		case EXCEPTION_IN_PAGE_ERROR:
			name = "EXCEPTION_IN_PAGE_ERROR";
			break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			name = "EXCEPTION_INT_DIVIDE_BY_ZERO";
			break;
		case EXCEPTION_INT_OVERFLOW:
			name = "EXCEPTION_INT_OVERFLOW";
			break;
		case EXCEPTION_INVALID_DISPOSITION:
			name = "EXCEPTION_INVALID_DISPOSITION";
			break;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			name = "EXCEPTION_NONCONTINUABLE_EXCEPTION";
			break;
		case EXCEPTION_PRIV_INSTRUCTION:
			name = "EXCEPTION_PRIV_INSTRUCTION";
			break;
		case EXCEPTION_SINGLE_STEP:
			name = "EXCEPTION_SINGLE_STEP";
			break;
		case EXCEPTION_STACK_OVERFLOW:
			name = "EXCEPTION_STACK_OVERFLOW";
			break;
		default:
			name = "Unknown Win32 Exception";
		}

		std::cout << "Process aborting due to: " << name << "\n";
		std::cout << "[start of stack trace]\n";
		Debug::printCallStackTo(std::cout, 4);
		std::cout << "[end of stack trace]\n";

		if (errorHandler) {
			errorHandler(name);
		}

		return EXCEPTION_CONTINUE_SEARCH;
	}
#endif
}

void Debug::setErrorHandling(const String& dumpFilePath, std::function<void(std::string_view)> eh)
{
	dumpFile = dumpFilePath;


#ifndef NN_NINTENDO_SDK
	std::set_terminate(&terminateHandler);
#endif

#if defined(WIN32)
	SetUnhandledExceptionFilter(&win32ExceptionHandler);
	//AddVectoredExceptionHandler(1, win32ExceptionHandler);
#endif

#if defined(HAS_SIGNAL)
	::signal(SIGSEGV, &signalHandler);
	::signal(SIGABRT, &signalHandler);
#endif

	errorHandler = std::move(eh);
}

void Debug::abort()
{
	std::cout << "Cleaning up debug setup...\n";

#ifndef NN_NINTENDO_SDK
	std::set_terminate(nullptr);
#endif

#if defined(WIN32)
	SetUnhandledExceptionFilter(nullptr);
#endif

#if defined(HAS_SIGNAL)
    ::signal(SIGSEGV, SIG_DFL);
	::signal(SIGABRT, SIG_DFL);
#endif

	std::cout << "Invoking std::abort()\n";
	std::abort();
}

void Debug::abort(std::string_view message)
{
	Logger::logError(message);

	OS::get().displayError(message);

	Debug::abort();
}


String Debug::getCallStack(int skip)
{
#if defined(HAS_STACKWALKER)

	std::stringstream ss;

	// NB: StackWalker isn't thread-safe - uses mutex if multiple sources are trying to retrieve
	// callstacks at the same time, for example in the Editor when hitting asset build errors.
	UniqueLock lock(mutex);
	OStreamStackWalker walker(ss, skip);
	walker.ShowCallstack();
	return ss.str();

#endif
}

void Debug::getCallStack(gsl::span<char> dst, int skip)
{
#if defined(HAS_STACKWALKER)
	NoAllocStackWalker walker(dst, skip);
	walker.ShowCallstack();
#endif
}

void Debug::printCallStackTo(std::ostream& out, int skip)
{
#if defined(HAS_STACKWALKER)
	OStreamStackWalker walker(out, skip);
	walker.ShowCallstack();
#endif
}

void Debug::trace(const char* filename, int line, std::string_view arg)
{
	auto& trace = lastTraces[tracePos.fetch_add(1) % lastTraces.size()];
	trace.filename = filename;
	trace.line = line;

	//Logger::logDev(String(filename) + ":" + line + " - " + (arg.empty() ? String() : String(arg)));

	if (!arg.empty()) {
		size_t len = std::min(trace.arg.size() - 1, arg.length());
		memcpy(trace.arg.data(), arg.data(), len);
		trace.arg[len] = 0;
	} else {
		trace.arg[0] = 0;
	}
}

String Debug::getLastTraces()
{
	std::stringstream result;
	const size_t n = lastTraces.size();
	const int startPos = tracePos;
	for (size_t i = 0; i < n; ++i) {
		auto& trace = lastTraces[(i + startPos) % n];
		result << " - " + String(trace.filename) + ":" + toString(trace.line);
		if (trace.arg[0] != 0) {
			result << String(" [") + trace.arg.data() + "]";
		}
		if (i == n - 1) {
			result << " [latest]";
		}
		result << "\n";
	}
	return result.str();
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
std::atomic<int> Debug::tracePos = 0;
Mutex Debug::mutex;
