#include "halley/support/logger.h"
#include "halley/text/halleystring.h"
#include <gsl/gsl_assert>
#include <iostream>
#include "halley/support/console.h"

using namespace Halley;

StdOutSink::StdOutSink(bool devMode)
	: devMode(devMode)
{
}

StdOutSink::~StdOutSink()
{
	std::cout.flush();
}

void StdOutSink::log(LoggerLevel level, const String& msg)
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

void Logger::log(LoggerLevel level, const String& msg)
{
	if (instance) {
		for (const auto& s: instance->sinks) {
			s->log(level, msg);
		}
	} else {
		std::cout << msg << '\n';
	}
}

void Logger::logTo(ILoggerSink* sink, LoggerLevel level, const String& msg)
{
	if (sink) {
		sink->log(level, msg);
	} else {
		log(level, msg);
	}
}

void Logger::logDev(const String& msg)
{
	log(LoggerLevel::Dev, msg);
}

void Logger::logInfo(const String& msg)
{
	log(LoggerLevel::Info, msg);
}

void Logger::logWarning(const String& msg)
{
	log(LoggerLevel::Warning, msg);
}

void Logger::logError(const String& msg)
{
	log(LoggerLevel::Error, msg);
}

void Logger::logException(const std::exception& e)
{
	logError(e.what());
}

Logger* Logger::instance = nullptr;
