#include "halley/concurrency/task.h"
#include "halley/text/halleystring.h"
#include <mutex>
#include <gsl/gsl_assert>
#include "halley/concurrency/concurrent.h"
#include <iostream>
#include "halley/support/logger.h"
#include <chrono>

using namespace Halley;

Task::Task(String name, bool isCancellable, bool isVisible, Vector<String> exclusivityTags) 
	: progress(0)
	, name(std::move(name))
	, cancelled(false)
	, hasPendingTasksOnQueue(false)
	, pendingTaskCount(0)
	, isCancellable(isCancellable)
	, isVisible(isVisible)
	, exclusivityTags(std::move(exclusivityTags))
{}

void Task::updateOnMain(float time)
{
}

void Task::addContinuation(std::unique_ptr<Task> task)
{
	std::lock_guard<std::mutex> lock(mutex);
	task->parent = parent;
	continuations.emplace_back(std::move(task));
}

void Task::clearTask(String name)
{
	std::lock_guard<std::mutex> lock(mutex);
	toClear.emplace_back(std::move(name));
}

void Task::setName(String name)
{
	this->name = std::move(name);
}

void Task::setProgress(float p)
{
	std::lock_guard<std::mutex> lock(mutex);
	progress = std::max(0.0f, std::min(p, 1.0f));
}

void Task::setProgress(float p, String label)
{
	std::lock_guard<std::mutex> lock(mutex);
	progress = std::max(0.0f, std::min(p, 1.0f));
	progressLabel = std::move(label);
}

void Task::setProgressLabel(String label)
{
	std::lock_guard<std::mutex> lock(mutex);
	progressLabel = std::move(label);
}

void Task::logDev(String message)
{
	log(LoggerLevel::Dev, std::move(message));
}

void Task::logInfo(String message)
{
	log(LoggerLevel::Info, std::move(message));
}

void Task::logWarning(String message)
{
	log(LoggerLevel::Warning, std::move(message));
}

void Task::logError(String message)
{
	log(LoggerLevel::Error, std::move(message));
}

void Task::log(LoggerLevel level, String message)
{
	Logger::log(level, name + "> " + message);

	std::lock_guard<std::mutex> lock(mutex);
	if (level == LoggerLevel::Error) {
		error = true;
	}
	++numMessages;
	messageLog.emplace_back(level, std::move(message));
}

bool Task::hasError() const
{
	return error;
}

size_t Task::getNumMessages() const
{
	return numMessages.load();
}

Vector<std::pair<LoggerLevel, String>> Task::copyMessagesHead(size_t max, std::optional<LoggerLevel> filter) const
{
	std::lock_guard<std::mutex> lock(mutex);

	Vector<std::pair<LoggerLevel, String>> result;
	for (const auto& msg: messageLog) {
		if (!filter || msg.first == filter.value()) {
			result.push_back(msg);
			if (result.size() >= max) {
				break;
			}
		}
	}
	
	return result;
}

Vector<std::pair<LoggerLevel, String>> Task::copyMessagesTail(size_t max, std::optional<LoggerLevel> filter) const
{
	Vector<std::pair<LoggerLevel, String>> result;

	{
		std::lock_guard<std::mutex> lock(mutex);
		for (auto iter = messageLog.rbegin(); iter != messageLog.rend(); ++iter) {
			if (!filter || iter->first == filter.value()) {
				result.push_back(*iter);
				if (result.size() >= max) {
					break;
				}
			}
		}
	}

	std::reverse(result.begin(), result.end());
	
	return result;
}

bool Task::isCancelled() const
{
	return cancelled;
}

void Task::setError(bool error)
{
	std::lock_guard<std::mutex> lock(mutex);
	this->error = error;
}

bool Task::hasPendingTasks() const
{
	return pendingTaskCount != 0;
}

void Task::addPendingTask(std::unique_ptr<Task> task)
{
	task->parent = this;
	std::lock_guard<std::mutex> lock(mutex);

	++pendingTaskCount;
	pendingTasks.emplace_back(std::move(task));
	hasPendingTasksOnQueue = true;
	Ensures(pendingTaskCount > 0);
}

void Task::onPendingTaskDone(size_t numContinuations)
{
	std::lock_guard<std::mutex> lock(mutex);
	Expects(pendingTaskCount > 0);
	pendingTaskCount += static_cast<int>(numContinuations) - 1;
	Ensures(pendingTaskCount >= 0);
}

Task* Task::getParent() const
{
	return parent;
}

void Task::setVisible(bool visible)
{
	isVisible = visible;
}

std::optional<String> Task::getAction()
{
	return {};
}

void Task::doAction(TaskSet& taskSet)
{
}
