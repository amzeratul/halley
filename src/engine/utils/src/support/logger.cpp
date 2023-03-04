#include "halley/support/logger.h"
#include "halley/text/halleystring.h"
#include <gsl/gsl_assert>
#include <iostream>
#include "halley/support/console.h"
#include "halley/utils/hash.h"

using namespace Halley;

StdOutSink::StdOutSink(bool devMode)
	: devMode(devMode)
{
}

StdOutSink::~StdOutSink()
{
	std::cout.flush();
}

void StdOutSink::log(LoggerLevel level, std::string_view msg)
{
	if (level == LoggerLevel::Dev && !devMode) {
		return;
	}

	std::unique_lock<std::mutex> lock(mutex);

	switch (level) {
	case LoggerLevel::Error:
		std::cout << ConsoleColour(Console::RED);
		break;
	case LoggerLevel::Warning:
		std::cout << ConsoleColour(Console::YELLOW);
		break;
	case LoggerLevel::Dev:
		std::cout << ConsoleColour(Console::CYAN);
		break;
	case LoggerLevel::Info:
		break;
	}
	std::cout << msg << ConsoleColour() << '\n';
}

void Logger::setInstance(Logger& logger)
{
	instance = &logger;
}

void Logger::addSink(ILoggerSink& sink)
{
	Expects(instance);
	instance->sinks.insert(&sink);
}

void Logger::removeSink(ILoggerSink& sink)
{
	Expects(instance);
	instance->sinks.erase(&sink);
}

void Logger::log(LoggerLevel level, std::string_view msg, bool once)
{
	if (instance) {
		if (once) {
			Hash::Hasher hasher;
			hasher.feed(level);
			hasher.feed(msg);
			const auto hash = hasher.digest();

			if (instance->logOnce.contains(hash)) {
				return;
			} else {
				instance->logOnce.emplace(hash);
			}
		}

		for (const auto& s: instance->sinks) {
			s->log(level, msg);
		}
	} else {
		std::cout << msg << '\n';
	}
}

void Logger::logTo(ILoggerSink* sink, LoggerLevel level, std::string_view msg)
{
	if (sink) {
		sink->log(level, msg);
	} else {
		log(level, msg);
	}
}

void Logger::logDev(std::string_view msg, bool once)
{
	log(LoggerLevel::Dev, msg, once);
}

void Logger::logInfo(std::string_view msg, bool once)
{
	log(LoggerLevel::Info, msg, once);
}

void Logger::logWarning(std::string_view msg, bool once)
{
	log(LoggerLevel::Warning, msg, once);
}

void Logger::logError(std::string_view msg, bool once)
{
	log(LoggerLevel::Error, msg, once);
}

void Logger::logException(const std::exception& e)
{
	logError(e.what());
}

Logger* Logger::instance = nullptr;
