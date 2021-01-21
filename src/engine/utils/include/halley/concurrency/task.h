#pragma once

#include <mutex>
#include <atomic>
#include <list>
#include <vector>

#include "halley/data_structures/vector.h"
#include "halley/text/halleystring.h"
#include "halley/concurrency/future.h"

namespace Halley
{
	enum class LoggerLevel;

	class TaskAnchor;

	class Task
	{
		friend class TaskAnchor;

	public:
		virtual ~Task() {}

	protected:
		Task(String name, bool isCancellable, bool isVisible);

		virtual void run() = 0;
		void addContinuation(std::unique_ptr<Task> task);

		void setName(String name);
		void setProgress(float progress, String label = "");

		void logDev(String message);
		void logInfo(String message);
		void logWarning(String message);
		void logError(String message);
		void log(LoggerLevel level, String message);
		
		bool isCancelled() const;
		bool hasError() const;

		size_t getNumMessages() const;
		std::vector<std::pair<LoggerLevel, String>> copyMessagesHead(size_t max, std::optional<LoggerLevel> filter = {}) const;
		std::vector<std::pair<LoggerLevel, String>> copyMessagesTail(size_t max, std::optional<LoggerLevel> filter = {}) const;

		bool hasPendingTasks() const;
		void addPendingTask(std::unique_ptr<Task> task);
		void onPendingTaskDone();

		Task* getParent() const;

	private:
		Vector<std::unique_ptr<Task>> continuations;
		Vector<std::unique_ptr<Task>> pendingTasks;

		mutable std::mutex mutex;
		std::atomic<float> progress;
		String name;
		String progressLabel;

		std::atomic<bool> cancelled;
		std::atomic<bool> hasPendingTasksOnQueue;
		std::atomic<int> pendingTaskCount;

		Task* parent = nullptr;

		const bool isCancellable;
		const bool isVisible;
		
		bool error = false;
		std::list<std::pair<LoggerLevel, String>> messageLog;
		std::atomic<size_t> numMessages;
	};
}
