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

#pragma once

#include "halley/text/halleystring.h"
#include <list>
#include <array>
#include <functional>
#include <atomic>

#include "halley/concurrency/mutex.h"

namespace Halley {
	class StackDebugTrace;

	class Debug {
		friend class StackDebugTrace;

	public:
		[[nodiscard]] static constexpr bool isDebug()
		{
		#ifdef _DEBUG
			return true;
		#else
			return false;
		#endif
		}

		static void setErrorHandling(const String& dumpFilePath, std::function<void(std::string_view)> errorHandler);
		[[nodiscard]] static String getCallStack(int skip = 3); // Thread safe
		[[nodiscard]] static std::string_view getCallStackUnsafe(gsl::span<char> dst, int skip = 3); // Not thread safe, use in unsafe environments
		[[nodiscard]] static std::string_view getCallStackUnsafe(int skip = 3); // Not thread safe, use in unsafe environments
		static void printCallStackToUnsafe(std::ostream& out, int skip); // Not thread safe, use in unsafe environments

		static void abort();
		static void abort(std::string_view message);

		static bool isRunningFromDLL();

	private:
		static bool debugging;
		static Mutex mutex;

		static thread_local Vector<const StackDebugTrace*> stackDebugTraces;

		Debug();

		static void registerDebugTrace(const StackDebugTrace& trace);
		static void unregisterDebugTrace(const StackDebugTrace& trace);
	};

	class StackDebugTrace {
	public:
		[[nodiscard]] StackDebugTrace(std::string_view name)
			: name(name)
			, type(Type::Undefined)
		{			
		}

		[[nodiscard]] StackDebugTrace(std::string_view name, std::string_view value)
			: name(name)
			, strValue(value)
			, type(Type::StringView)
		{
			doRegister();
		}

		[[nodiscard]] StackDebugTrace(std::string_view name, int64_t value)
			: name(name)
			, type(Type::Int64)
		{
			this->value.int64Value = value;
			doRegister();
		}

		[[nodiscard]] StackDebugTrace(std::string_view name, double value)
			: name(name)
			, type(Type::Double)
		{
			this->value.doubleValue = value;
			doRegister();
		}

		[[nodiscard]] StackDebugTrace(std::string_view name, void* value)
			: name(name)
			, type(Type::Pointer)
		{
			this->value.ptrValue = value;
			doRegister();
		}

#if defined(DEV_BUILD) || defined(_DEBUG)
		~StackDebugTrace()
		{
			if (type != Type::Undefined) {
				Debug::unregisterDebugTrace(*this);
			}
		}
#endif

		StackDebugTrace(const StackDebugTrace& other) = delete;
		StackDebugTrace(StackDebugTrace&& other) = delete;
		StackDebugTrace& operator=(const StackDebugTrace& other) = delete;
		StackDebugTrace& operator=(StackDebugTrace&& other) = delete;

		[[nodiscard]] std::string_view getName() const { return name; }
		[[nodiscard]] bool isString() const { return type == Type::StringView; }
		[[nodiscard]] std::string_view getValue(gsl::span<char> buffer) const;

		void setValue(std::string_view value)
		{
			if (this->type == Type::Undefined) {
				doRegister();
			}
			this->type = Type::StringView;
			this->strValue = value;
		}

		void setValue(int64_t value)
		{
			if (this->type == Type::Undefined) {
				doRegister();
			}
			this->type = Type::Int64;
			this->value.int64Value = value;
		}

		void setValue(double value)
		{
			if (this->type == Type::Undefined) {
				doRegister();
			}
			this->type = Type::Double;
			this->value.doubleValue = value;
		}

		void setValue(void* value)
		{
			if (this->type == Type::Undefined) {
				doRegister();
			}
			this->type = Type::Pointer;
			this->value.ptrValue = value;
		}

		void* operator new(size_t) = delete;
		void* operator new(size_t, void*) = delete;

	private:
		enum class Type : uint8_t {
			Undefined,
			StringView,
			Int64,
			Double,
			Pointer
		};

		std::string_view name;
		std::string_view strValue;
		union {
			int64_t int64Value;
			double doubleValue;
			void* ptrValue;
		} value;
		Type type;

		void doRegister() const
		{
#if defined(DEV_BUILD) || defined(_DEBUG)
			Debug::registerDebugTrace(*this);
#endif
		}
	};
}
