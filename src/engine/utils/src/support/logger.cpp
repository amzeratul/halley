#include "halley/support/logger.h"
#include "halley/text/halleystring.h"
#include <gsl/gsl_assert>
#include <iostream>

using namespace Halley;

void StdOutSink::log(LoggerLevel level, const String& msg)
{
	std::cout << msg << std::endl;
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
		for (auto& s: instance->sinks) {
			s->log(level, msg);
		}
	} else {
		std::cout << msg << std::endl;
	}
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
