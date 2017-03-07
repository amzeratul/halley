#include "halley/tools/tasks/editor_task.h"
#include "halley/text/halleystring.h"
#include <mutex>
#include <gsl/gsl_assert>
#include "halley/concurrency/concurrent.h"
#include <iostream>
#include "halley/support/logger.h"

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

void EditorTask::addContinuation(EditorTaskAnchor&& task)
{
	continuations.emplace_back(std::move(task));
}

void EditorTask::setContinuations(Vector<EditorTaskAnchor>&& tasks)
{
	continuations = std::move(tasks);
}

void EditorTask::setProgress(float p, String label)
{
	std::lock_guard<std::mutex> lock(mutex);
	progress = std::max(0.0f, std::min(p, 1.0f));
	progressLabel = label;
}

void EditorTask::addError(const String& message)
{
	Logger::logError("Error importing asset: " + message);

	std::lock_guard<std::mutex> lock(mutex);
	error = true;
	if (!errorMsg.isEmpty()) {
		errorMsg += "\n";
	}
	errorMsg += message;
}

bool EditorTask::isCancelled() const
{
	return cancelled;
}

bool EditorTask::hasError() const
{
	return error;
}

const String& EditorTask::getError() const
{
	return errorMsg;
}

bool EditorTask::hasPendingTasks() const
{
	return pendingTaskCount != 0;
}

void EditorTask::addPendingTask(EditorTaskAnchor&& task)
{
	task.setParent(*this);
	std::lock_guard<std::mutex> lock(mutex);

	++pendingTaskCount;
	pendingTasks.emplace_back(std::move(task));
	hasPendingTasksOnQueue = true;
	Ensures(pendingTaskCount > 0);
}

void EditorTask::onPendingTaskDone(const EditorTaskAnchor& editorTaskAnchor)
{
	Expects(pendingTaskCount > 0);
	std::lock_guard<std::mutex> lock(mutex);
	--pendingTaskCount;
	Ensures(pendingTaskCount >= 0);
}

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
				while (status == EditorTaskStatus::Started && !taskFuture.hasValue()) {}
			}

			if (parent) {
				parent->onPendingTaskDone(*this);
			}
		}
	}
}

void EditorTaskAnchor::update(float time)
{
	if (status == EditorTaskStatus::WaitingToStart) {
		timeToStart -= time;
		if (timeToStart <= 0) {
			taskFuture = Concurrent::execute(Task<void>([this]() { task->run(); }));
			status = EditorTaskStatus::Started;
		}
	} else if (status == EditorTaskStatus::Started) {
		bool done = taskFuture.hasValue();
		if (done) {
			status = EditorTaskStatus::Done;
			error = task->hasError();
			errorMsg = task->getError();
			progress = 1;
			progressLabel = "";
		} else {
			std::lock_guard<std::mutex> lock(task->mutex);
			progress = task->progress;
			progressLabel = task->progressLabel;
		}
	}
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

const String& EditorTaskAnchor::getError() const
{
	return errorMsg;
}

Vector<EditorTaskAnchor> EditorTaskAnchor::getContinuations()
{
	return std::move(task->continuations);
}

Vector<EditorTaskAnchor> EditorTaskAnchor::getPendingTasks()
{
	if (task->hasPendingTasksOnQueue) {
		std::lock_guard<std::mutex> lock(task->mutex);
		if (task->hasPendingTasksOnQueue) {
			task->hasPendingTasksOnQueue = false;
			Vector<EditorTaskAnchor> result = std::move(task->pendingTasks);
			task->pendingTasks.clear();
			return result;
		}
	}
	
	return Vector<EditorTaskAnchor>();
}

void EditorTaskAnchor::setParent(EditorTask& editorTask)
{
	parent = &editorTask;
}
