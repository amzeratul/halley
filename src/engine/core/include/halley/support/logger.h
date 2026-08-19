#pragma once

#include <exception>
#include <set>
#include <thread>
#include "../concurrency/mutex.h"

#include "halley/data_structures/hash_map.h"
#include "halley/data_structures/ring_buffer.h"
#include "halley/text/enum_names.h"

namespace Halley
{
	class SystemAPI;
	class String;

	enum class LoggerLevel
	{
		Dev,
		Info,
		Warning,
		Error
	};

	template <>
	struct EnumNames<LoggerLevel> {
		constexpr auto operator()() const {
			return std::to_array({
				"dev",
				"info",
				"warning",
				"error"
			});
		}
	};

	class ILoggerSink
	{
	public:
		virtual ~ILoggerSink() {}
		virtual void log(LoggerLevel level, std::string_view msg) {}
		virtual void log(LoggerLevel level, std::string_view msg, bool isInterrupt)
		{
			if (canLogInContext(isInterrupt)) {
				log(level, msg);
			}
		}
		virtual bool canLogInContext(bool isInterrupt) { return !isInterrupt; }
		virtual void setInterruptContext() {}
	};

	class StdOutSink final : public ILoggerSink {
	public:
		explicit StdOutSink(bool devMode, bool forceFlush = false);
		~StdOutSink();
		void log(LoggerLevel level, std::string_view msg) override;
		bool canLogInContext(bool isInterrupt) override;
		void setInterruptContext() override;

	private:
		Mutex mutex;
		bool devMode = false;
		bool forceFlush = false;
		bool interruptContext = false;
	};

	class ThreadedLogger final : public ILoggerSink {
	public:
		ThreadedLogger();
		~ThreadedLogger() override;

		void setDevMode(bool devMode);
		void createBasicThread();
		void createSystemThread(SystemAPI& system);

		void log(LoggerLevel level, std::string_view msg, bool isInterrupt) override;

	private:
		struct Entry {
			LoggerLevel level;
			String msg;
		};

		RingBuffer<Entry> pendingEntries;
		std::thread thread;
		std::atomic<bool> running;
		mutable Mutex writeMutex;
		bool devMode = false;

		void run();
		void stopThread();
		void doLog(LoggerLevel level, std::string_view msg);
	};

	class Logger
	{
	public:
		static void setInstance(Logger& logger);

		static void addSink(ILoggerSink& sink);
		static void removeSink(ILoggerSink& sink);

		static void log(LoggerLevel level, std::string_view msg, bool once = false);
		static void logTo(ILoggerSink* sink, LoggerLevel level, std::string_view msg);
		static void logDev(std::string_view msg, bool once = false);
		static void logInfo(std::string_view msg, bool once = false);
		static void logWarning(std::string_view msg, bool once = false);
		static void logError(std::string_view msg, bool once = false);
		static void logException(const std::exception& e);
		static void setInterruptContext();

	private:
		static Logger* instance;

		std::set<ILoggerSink*> sinks;
		HashSet<uint64_t> logOnce;
		bool interruptContext = false;
	};

	class IScreenLogger {
	public:
	    virtual ~IScreenLogger() = default;
	    virtual void onLog(std::string_view key, String value, double time = 0) = 0;
	    virtual void onCounter(std::string_view key) = 0;
	};

	class ScreenLogger {
	public:
		static void logScreen(std::string_view key, String value, double time = 0);
		static void logScreen(std::string_view key, int value, double time = 0);
		static void logScreen(std::string_view key, float value, double time = 0);

		static void logScreenCounter(std::string_view key);

	    static void setTarget(IScreenLogger* target);

	private:
	    static IScreenLogger* target;
	};

}
