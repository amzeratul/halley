#include "editor_task.h"

using namespace Halley;

EditorTask::EditorTask(String name, bool isCancellable, bool isVisible) 
	: progress(0)
	, name(name)
	, cancelled(false)
	, isCancellable(isCancellable)
	, isVisible(isVisible)
{}

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

bool EditorTask::isCancelled() const
{
	return cancelled;
}

EditorTaskAnchor::EditorTaskAnchor(std::unique_ptr<EditorTask> task, float delay)
	: task(std::move(task))
	, status(EditorTaskStatus::WaitingToStart)
	, timeToStart(delay) {}

EditorTaskAnchor::EditorTaskAnchor(EditorTaskAnchor&& other) = default;

EditorTaskAnchor::~EditorTaskAnchor()
{
	// Wait for task to join
	cancel();
	while (status == EditorTaskStatus::Started && !taskFuture.hasValue()) {}
}

EditorTaskAnchor& EditorTaskAnchor::operator=(EditorTaskAnchor&& other) = default;

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
	if (task->isCancellable) {
		task->cancelled = true;
	}
}

Vector<EditorTaskAnchor> EditorTaskAnchor::getContinuations()
{
	return std::move(task->continuations);
}
