#include "halley/concurrency/task_anchor.h"
#include "halley/text/halleystring.h"
#include <mutex>
#include <gsl/gsl_assert>
#include "halley/concurrency/concurrent.h"
#include <iostream>
#include "halley/support/logger.h"
#include <chrono>

using namespace Halley;

TaskAnchor::TaskAnchor(std::unique_ptr<Task> t, float delay)
	: task(std::move(t))
	, status(TaskStatus::WaitingToStart)
	, timeToStart(delay)
{
	Expects(!!task);
}

TaskAnchor::TaskAnchor(TaskAnchor&& other) noexcept = default;
TaskAnchor& TaskAnchor::operator=(TaskAnchor&& other) noexcept = default;

TaskAnchor::~TaskAnchor()
{
	terminate();
}

void TaskAnchor::terminate()
{
	if (!terminated) {
		terminated = true;

		// If this has been moved, task will be null
		if (task) {
			// Wait for task to join
			if (status != TaskStatus::Done) {
				cancel();
				while (status == TaskStatus::Started && !taskFuture.hasValue()) {
					using namespace std::chrono_literals;
					std::this_thread::sleep_for(500ms);
					if (status == TaskStatus::Started && !taskFuture.hasValue()) {
						Logger::logInfo("Waiting for task thread to join...");
					}
				}
			}

			auto* parent = task->getParent();
			if (parent) {
				parent->onPendingTaskDone();
			}
		}
	}
}

void TaskAnchor::update(float time)
{
	if (status == TaskStatus::WaitingToStart) {
		timeToStart -= time;
		if (timeToStart <= 0) {
			status = TaskStatus::Started;
			taskFuture = Concurrent::execute([this]() { task->run(); });
		}
	} else if (status == TaskStatus::Started) {
		bool done = taskFuture.hasValue();
		if (done) {
			status = TaskStatus::Done;
			error = task->hasError();
			progress = 1;
			progressLabel = "";
		} else {
			std::lock_guard<std::mutex> lock(task->mutex);
			progress = task->progress;
			progressLabel = task->progressLabel;
		}
	}
}

TaskStatus TaskAnchor::getStatus() const
{
	return status;
}

String TaskAnchor::getName() const
{
	return task->name;
}

String TaskAnchor::getProgressLabel() const
{
	return progressLabel;
}

bool TaskAnchor::canCancel() const
{
	return task->isCancellable;
}

bool TaskAnchor::isVisible() const
{
	return task->isVisible;
}

void TaskAnchor::cancel()
{
	if (status == TaskStatus::WaitingToStart) {
		status = TaskStatus::Done;
	}
	if (task->isCancellable) {
		task->cancelled = true;
	}
}

void TaskAnchor::setId(int value)
{
	id = value;
}

bool TaskAnchor::hasError() const
{
	return error;
}

size_t TaskAnchor::getNumMessages() const
{
	return task->getNumMessages();
}

std::vector<std::pair<LoggerLevel, String>> TaskAnchor::copyMessagesHead(size_t max, std::optional<LoggerLevel> filter) const
{
	return task->copyMessagesHead(max, filter);
}

std::vector<std::pair<LoggerLevel, String>> TaskAnchor::copyMessagesTail(size_t max, std::optional<LoggerLevel> filter) const
{
	return task->copyMessagesTail(max, filter);
}

Vector<std::unique_ptr<Task>> TaskAnchor::getContinuations()
{
	std::lock_guard<std::mutex> lock(task->mutex);
	return std::move(task->continuations);
}

Vector<std::unique_ptr<Task>> TaskAnchor::getPendingTasks()
{
	if (task->hasPendingTasksOnQueue) {
		std::lock_guard<std::mutex> lock(task->mutex);
		if (task->hasPendingTasksOnQueue) {
			task->hasPendingTasksOnQueue = false;
			Vector<std::unique_ptr<Task>> result = std::move(task->pendingTasks);
			task->pendingTasks.clear();
			return result;
		}
	}
	
	return {};
}
