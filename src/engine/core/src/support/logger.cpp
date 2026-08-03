#include "halley/support/logger.h"
#include "halley/text/halleystring.h"
#include "halley/support/assert.h"
#include <iostream>
#include "halley/support/console.h"
#include "halley/utils/hash.h"

using namespace Halley;

StdOutSink::StdOutSink(bool devMode, bool forceFlush)
	: devMode(devMode)
	, forceFlush(forceFlush)
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

	UniqueLock lock(mutex, std::defer_lock_t());
	if (!interruptContext) {
		lock.lock();
	}

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
	std::cout << msg << ConsoleColour();
	if (forceFlush) {
		std::cout << std::endl;
	} else {
		std::cout << "\n";
	}
}

bool StdOutSink::canLogInInterruptContext()
{
	return true;
}

void StdOutSink::setInterruptContext()
{
	interruptContext = true;
}

void Logger::setInstance(Logger& logger)
{
	instance = &logger;
}

void Logger::addSink(ILoggerSink& sink)
{
	HalleyAssertDev(instance);
	if (!instance->interruptContext) {
		instance->sinks.insert(&sink);
	}
}

void Logger::removeSink(ILoggerSink& sink)
{
	HalleyAssertDev(instance);
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
			if (!instance->interruptContext || s->canLogInInterruptContext()) {
				s->log(level, msg);
			}
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

void Logger::setInterruptContext()
{
	instance->interruptContext = true;
	for (auto& sink: instance->sinks) {
		sink->setInterruptContext();
	}
}

Logger* Logger::instance = nullptr;

void ScreenLogger::logScreen(std::string_view key, String value, double time)
{
	if (target) {
		target->onLog(key, std::move(value), time);
	}
}

void ScreenLogger::logScreen(std::string_view key, int value, double time)
{
	logScreen(key, toString(value), time);
}

void ScreenLogger::logScreen(std::string_view key, float value, double time)
{
	logScreen(key, toString(value), time);	
}

void ScreenLogger::logScreenCounter(std::string_view key)
{
	if (target) {
		target->onCounter(key);
	}
}

void ScreenLogger::setTarget(IScreenLogger* t)
{
	target = t;
}

IScreenLogger* ScreenLogger::target = nullptr;
