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
#endif

#if defined(HAS_STACKWALKER)
#include "StackWalker/StackWalker.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

namespace Halley {
	class NoAllocStackWalker : public StackWalker {
	public:
		NoAllocStackWalker(gsl::span<char> dst, int startFrom, gsl::span<const StackDebugTrace*> traces)
			: dst(dst)
			, traces(traces)
			, lastTrace(traces.size())
			, startFrom(startFrom)
		{}

		std::string_view getResult() const
		{
			return std::string_view(dst.data(), curWritePos);
		}

	protected:
		void OnCallstackEntry(CallstackEntryType eType, CallstackEntry& entry) override
		{
			// Print stack traces that happened on frames above this
			printTraces(entry.stackPointer);

			if (eType == firstEntry) {
				curPos = 0;
			}

			auto entryName = std::string_view(entry.name);
			if (++curPos < startFrom || (displayIndex == 0 && (entryName.starts_with("Halley::Exception") || entryName.starts_with("Halley::Debug")))) {
				return;
			}

			const auto index = displayIndex++;
			cat(index < 10 ? "  " : " ");
			cat(index);
			cat(": ");
			cat(entryName);
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
		
	private:
		gsl::span<char> dst;
		size_t curWritePos = 0;

		gsl::span<const StackDebugTrace*> traces;
		size_t lastTrace = 0;

		int startFrom = 0;
		int curPos = 0;
		int displayIndex = 0;

		void cat(std::string_view str)
		{
			if (!str.empty()) {
				const size_t toWrite = std::min(str.length(), dst.size() - curWritePos);
				memcpy(dst.data() + curWritePos, str.data(), toWrite);
				curWritePos += toWrite;
			}
		}

		void cat(int value)
		{
			char buffer[32];
			_itoa(value, buffer, 10);
			cat(buffer);
		}

		void cat(uint64_t value)
		{
			char buffer[32];
			_i64toa(value, buffer, 16);
			cat(buffer);
		}

		void printTraces(size_t stackPtr)
		{
			char buffer[32];
			while (lastTrace > 0 && reinterpret_cast<size_t>(traces[lastTrace - 1]) < stackPtr) {
				const auto* trace = traces[--lastTrace];
				cat("    + ");
				cat(trace->getName());
				cat(trace->isString() ? ": \"" : ": ");
				cat(trace->getValue(buffer));
				cat(trace->isString() ? "\"\n" : "\n");
			}
		}
	};
}

#endif

#if !defined(NN_NINTENDO_SDK) && !defined(__ORBIS__) && !defined(__PROSPERO__)
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
		Debug::printCallStackToUnsafe(std::cout, 4);
		std::cout << "[end of stack trace]\n";

		if (errorHandler) {
			errorHandler(name);
		}

		Debug::abort();
	}
#endif

	[[maybe_unused]] void terminateHandler()
	{
		std::cout << "std::terminate() invoked.\n";
		std::cout << "[start of stack trace]\n";
		Debug::printCallStackToUnsafe(std::cout, 4);
		std::cout << "[end of stack trace]\n";

		errorHandler("std::terminate() invoked.");

		Debug::abort();
	}

#if defined(HAS_STACKWALKER)
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
		Debug::printCallStackToUnsafe(std::cout, 4);
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

#if defined(HAS_STACKWALKER)
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

#if defined(HAS_STACKWALKER)
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




#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace {
	HMODULE getCurrentModuleHandle()
	{
		HMODULE hMod = nullptr;
		GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&getCurrentModuleHandle), &hMod);
		return hMod;
	}

	constexpr bool checkForLoadingFromDLL = false;
}

bool Debug::isRunningFromDLL()
{
	if (!checkForLoadingFromDLL) {
		return false;
	}

#ifdef DEV_BUILD
	char name[1024];
	GetModuleFileNameA(getCurrentModuleHandle(), name, sizeof(name));
	auto str = std::string_view(name);
	return str.length() > 4 && str.substr(str.length() - 4, 4) == ".dll" && str.find("plugins") == std::string_view::npos;
#else
	return false;
#endif
}

#else

bool Debug::isRunningFromDLL()
{
	return false;
}

#endif

void Debug::registerDebugTrace(const StackDebugTrace& trace)
{
	stackDebugTraces += &trace;
}

void Debug::unregisterDebugTrace(const StackDebugTrace& trace)
{
	HalleyAssertDebug(stackDebugTraces.back() == &trace);
	stackDebugTraces.pop_back();
}

String Debug::getCallStack(int skip)
{
#if defined(HAS_STACKWALKER)
	// NB: StackWalker isn't thread-safe - uses mutex if multiple sources are trying to retrieve
	// callstacks at the same time, for example in the Editor when hitting asset build errors.
	UniqueLock lock(mutex);
	return String(getCallStackUnsafe(skip));
#else
	return {};
#endif
}

std::string_view Debug::getCallStackUnsafe(int skip)
{
#if defined(HAS_STACKWALKER)
	char buffer[64 * 1024]; // As Bill Gates once famously said...
	return getCallStackUnsafe(buffer, skip);
#else
	return {};
#endif
}

std::string_view Debug::getCallStackUnsafe(gsl::span<char> dst, int skip)
{
#if defined(HAS_STACKWALKER)
	NoAllocStackWalker walker(dst, skip, stackDebugTraces);
	walker.ShowCallstack();
	return walker.getResult();
#else
	return {};
#endif
}

void Debug::printCallStackToUnsafe(std::ostream& out, int skip)
{
#if defined(HAS_STACKWALKER)
	out << getCallStackUnsafe(skip);
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

std::string_view StackDebugTrace::getValue(gsl::span<char> buffer) const
{
	buffer[0] = 0;
	switch (type) {
	case Type::StringView:
		return strValue;
	case Type::Int64:
		(void) snprintf(buffer.data(), buffer.size(), "%lli", value.int64Value);
		return std::string_view(buffer.data(), strlen(buffer.data()));
	case Type::Double:
		(void) snprintf(buffer.data(), buffer.size(), "%f", value.doubleValue);
		return std::string_view(buffer.data(), strlen(buffer.data()));
	}
	return {};
}

std::array<Debug::DebugTraceEntry, 32> Debug::lastTraces;
std::atomic<int> Debug::tracePos = 0;
Mutex Debug::mutex;
thread_local Vector<const StackDebugTrace*> Debug::stackDebugTraces;
