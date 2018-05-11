#pragma once

#include <exception>
#include <set>
#include <mutex>

namespace Halley
{
	class String;

	enum class LoggerLevel
	{
		Dev,
		Info,
		Warning,
		Error
	};

	class ILoggerSink
	{
	public:
		virtual ~ILoggerSink() {}
		virtual void log(LoggerLevel level, const String& msg) = 0;
	};

	class StdOutSink : public ILoggerSink {
	public:
		explicit StdOutSink(bool devMode);
		void log(LoggerLevel level, const String& msg) override;

	private:
		std::mutex mutex;
		bool devMode;
	};

	class Logger
	{
	public:
		static void setInstance(Logger& logger);

		static void addSink(ILoggerSink& sink);
		static void removeSink(ILoggerSink& sink);

		static void log(LoggerLevel level, const String& msg);
		static void logDev(const String& msg);
		static void logInfo(const String& msg);
		static void logWarning(const String& msg);
		static void logError(const String& msg);
		static void logException(const std::exception& e);

	private:
		static Logger* instance;

		std::set<ILoggerSink*> sinks;
	};
}
