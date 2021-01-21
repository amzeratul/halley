#include "halley/concurrency/task_anchor.h"
#include "halley/text/halleystring.h"
#include <mutex>
#include <gsl/gsl_assert>
#include "halley/concurrency/concurrent.h"
#include <iostream>
#include "halley/support/logger.h"
#include <chrono>

using namespace Halley;

EditorTaskAnchor::EditorTaskAnchor(std::unique_ptr<EditorTask> t, float delay)
	: task(std::move(t))
	, status(EditorTaskStatus::WaitingToStart)
	, timeToStart(delay)
{
	Expects(!!task);
}

EditorTaskAnchor::EditorTaskAnchor(EditorTaskAnchor&& other) noexcept = default;
EditorTaskAnchor& EditorTaskAnchor::operator=(EditorTaskAnchor&& other) noexcept = default;

EditorTaskAnchor::~EditorTaskAnchor()
{
	terminate();
}

void EditorTaskAnchor::terminate()
{
	if (!terminated) {
		terminated = true;

		// If this has been moved, task will be null
		if (task) {
			// Wait for task to join
			if (status != EditorTaskStatus::Done) {
				cancel();
				while (status == EditorTaskStatus::Started && !taskFuture.hasValue()) {
					using namespace std::chrono_literals;
					std::this_thread::sleep_for(500ms);
					if (status == EditorTaskStatus::Started && !taskFuture.hasValue()) {
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

void EditorTaskAnchor::update(float time)
{
	if (status == EditorTaskStatus::WaitingToStart) {
		timeToStart -= time;
		if (timeToStart <= 0) {
			status = EditorTaskStatus::Started;
			taskFuture = Concurrent::execute([this]() { task->run(); });
		}
	} else if (status == EditorTaskStatus::Started) {
		bool done = taskFuture.hasValue();
		if (done) {
			status = EditorTaskStatus::Done;
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

EditorTaskStatus EditorTaskAnchor::getStatus() const
{
	return status;
}

String EditorTaskAnchor::getName() const
{
	return task->name;
}

String EditorTaskAnchor::getProgressLabel() const
{
	return progressLabel;
}

bool EditorTaskAnchor::canCancel() const
{
	return task->isCancellable;
}

bool EditorTaskAnchor::isVisible() const
{
	return task->isVisible;
}

void EditorTaskAnchor::cancel()
{
	if (status == EditorTaskStatus::WaitingToStart) {
		status = EditorTaskStatus::Done;
	}
	if (task->isCancellable) {
		task->cancelled = true;
	}
}

void EditorTaskAnchor::setId(int value)
{
	id = value;
}

bool EditorTaskAnchor::hasError() const
{
	return error;
}

size_t EditorTaskAnchor::getNumMessages() const
{
	return task->getNumMessages();
}

std::vector<std::pair<LoggerLevel, String>> EditorTaskAnchor::copyMessagesHead(size_t max, std::optional<LoggerLevel> filter) const
{
	return task->copyMessagesHead(max, filter);
}

std::vector<std::pair<LoggerLevel, String>> EditorTaskAnchor::copyMessagesTail(size_t max, std::optional<LoggerLevel> filter) const
{
	return task->copyMessagesTail(max, filter);
}

Vector<std::unique_ptr<EditorTask>> EditorTaskAnchor::getContinuations()
{
	std::lock_guard<std::mutex> lock(task->mutex);
	return std::move(task->continuations);
}

Vector<std::unique_ptr<EditorTask>> EditorTaskAnchor::getPendingTasks()
{
	if (task->hasPendingTasksOnQueue) {
		std::lock_guard<std::mutex> lock(task->mutex);
		if (task->hasPendingTasksOnQueue) {
			task->hasPendingTasksOnQueue = false;
			Vector<std::unique_ptr<EditorTask>> result = std::move(task->pendingTasks);
			task->pendingTasks.clear();
			return result;
		}
	}
	
	return {};
}
