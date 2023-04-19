#pragma once

#include <exception>
#include <set>
#include <mutex>

#include "halley/data_structures/hash_map.h"
#include "halley/text/enum_names.h"

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

	template <>
	struct EnumNames<LoggerLevel> {
		constexpr std::array<const char*, 4> operator()() const {
			return { {
				"dev",
				"info",
				"warning",
				"error"
			} };
		}
	};

	class ILoggerSink
	{
	public:
		virtual ~ILoggerSink() {}
		virtual void log(LoggerLevel level, std::string_view msg) = 0;
	};

	class StdOutSink final : public ILoggerSink {
	public:
		explicit StdOutSink(bool devMode, bool forceFlush = false);
		~StdOutSink();
		void log(LoggerLevel level, std::string_view msg) override;

	private:
		std::mutex mutex;
		bool devMode;
		bool forceFlush;
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

	private:
		static Logger* instance;

		std::set<ILoggerSink*> sinks;
		HashSet<uint64_t> logOnce;
	};
}
