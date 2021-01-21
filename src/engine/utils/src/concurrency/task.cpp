#include "halley/concurrency/task.h"
#include "halley/text/halleystring.h"
#include <mutex>
#include <gsl/gsl_assert>
#include "halley/concurrency/concurrent.h"
#include <iostream>
#include "halley/support/logger.h"
#include <chrono>

using namespace Halley;

EditorTask::EditorTask(String name, bool isCancellable, bool isVisible) 
	: progress(0)
	, name(name)
	, cancelled(false)
	, hasPendingTasksOnQueue(false)
	, pendingTaskCount(0)
	, isCancellable(isCancellable)
	, isVisible(isVisible)
{}

void EditorTask::addContinuation(std::unique_ptr<EditorTask> task)
{
	std::lock_guard<std::mutex> lock(mutex);
	continuations.emplace_back(std::move(task));
}

void EditorTask::setName(String name)
{
	this->name = std::move(name);
}

void EditorTask::setProgress(float p, String label)
{
	std::lock_guard<std::mutex> lock(mutex);
	progress = std::max(0.0f, std::min(p, 1.0f));
	progressLabel = label;
}

void EditorTask::logDev(String message)
{
	log(LoggerLevel::Dev, std::move(message));
}

void EditorTask::logInfo(String message)
{
	log(LoggerLevel::Info, std::move(message));
}

void EditorTask::logWarning(String message)
{
	log(LoggerLevel::Warning, std::move(message));
}

void EditorTask::logError(String message)
{
	log(LoggerLevel::Error, std::move(message));
}

void EditorTask::log(LoggerLevel level, String message)
{
	Logger::log(level, name + "> " + message);

	std::lock_guard<std::mutex> lock(mutex);
	if (level == LoggerLevel::Error) {
		error = true;
	}
	++numMessages;
	messageLog.emplace_back(level, std::move(message));
}

bool EditorTask::hasError() const
{
	return error;
}

size_t EditorTask::getNumMessages() const
{
	return numMessages.load();
}

std::vector<std::pair<LoggerLevel, String>> EditorTask::copyMessagesHead(size_t max, std::optional<LoggerLevel> filter) const
{
	std::lock_guard<std::mutex> lock(mutex);

	std::vector<std::pair<LoggerLevel, String>> result;
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

std::vector<std::pair<LoggerLevel, String>> EditorTask::copyMessagesTail(size_t max, std::optional<LoggerLevel> filter) const
{
	std::vector<std::pair<LoggerLevel, String>> result;

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

bool EditorTask::isCancelled() const
{
	return cancelled;
}

bool EditorTask::hasPendingTasks() const
{
	return pendingTaskCount != 0;
}

void EditorTask::addPendingTask(std::unique_ptr<EditorTask> task)
{
	task->parent = this;
	std::lock_guard<std::mutex> lock(mutex);

	++pendingTaskCount;
	pendingTasks.emplace_back(std::move(task));
	hasPendingTasksOnQueue = true;
	Ensures(pendingTaskCount > 0);
}

void EditorTask::onPendingTaskDone()
{
	std::lock_guard<std::mutex> lock(mutex);
	Expects(pendingTaskCount > 0);
	--pendingTaskCount;
	Ensures(pendingTaskCount >= 0);
}

EditorTask* EditorTask::getParent() const
{
	return parent;
}

