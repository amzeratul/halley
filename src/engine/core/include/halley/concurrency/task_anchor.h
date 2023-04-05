#pragma once

#include "halley/data_structures/vector.h"
#include "halley/text/halleystring.h"
#include "halley/concurrency/future.h"

namespace Halley
{
	enum class LoggerLevel;

	enum class TaskStatus
	{
		WaitingToStart,
		ReadyToStart,
		Started,
		Done
	};

	class Task;
	class TaskSet;
	class TaskExclusivityHandle;

	class TaskAnchor
	{
	public:
		TaskAnchor(std::unique_ptr<Task> task, float delay = 0);
		TaskAnchor(TaskAnchor&& other) noexcept;
		TaskAnchor(const TaskAnchor& other) = delete;
		~TaskAnchor();

		TaskAnchor& operator=(TaskAnchor&& other) noexcept;
		TaskAnchor& operator=(const TaskAnchor& other) = delete;

		void terminate(size_t numContinuations);
		void update(TaskSet& taskSet, float time);

		TaskStatus getStatus() const;
		String getName() const;
		String getProgressLabel() const;
		float getProgress() const { return progress; }

		bool canCancel() const;
		bool isVisible() const;
		void cancel();

		int getId() const { return id; }
		void setId(int value);

		bool hasError() const;
		
		size_t getNumMessages() const;
		Vector<std::pair<LoggerLevel, String>> copyMessagesHead(size_t max, std::optional<LoggerLevel> filter = {}) const;
		Vector<std::pair<LoggerLevel, String>> copyMessagesTail(size_t max, std::optional<LoggerLevel> filter = {}) const;

		Vector<std::unique_ptr<Task>> getContinuations();
		Vector<std::unique_ptr<Task>> getPendingTasks();

		std::optional<String> getAction();
		virtual void doAction(TaskSet& taskSet);

	private:
		std::unique_ptr<Task> task;
		Future<void> taskFuture;

		TaskStatus status;
		float timeToStart = 0;
		float progress = 0;
		bool terminated = false;
		bool error = false;
		String progressLabel;

		int id = 0;

		std::unique_ptr<TaskExclusivityHandle> exclusivityHandle;
		String waitingFor;
	};
}
