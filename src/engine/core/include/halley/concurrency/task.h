#pragma once

#include <mutex>
#include <atomic>
#include <list>
#include "halley/data_structures/vector.h"

#include "halley/data_structures/vector.h"
#include "halley/text/halleystring.h"
#include "halley/concurrency/future.h"

namespace Halley
{
	class TaskSet;
	enum class LoggerLevel;

	class TaskAnchor;

	class Task
	{
		friend class TaskAnchor;

	public:
		virtual ~Task() {}

	protected:
		Task(String name, bool isCancellable, bool isVisible, Vector<String> exclusivityTags = {});

		virtual void run() = 0;
		virtual void updateOnMain(float time);
		void addContinuation(std::unique_ptr<Task> task);
		void addReplacement(std::unique_ptr<Task> task);
		void clearTask(String name);

		void setName(String name);
		void setProgress(float progress);
		void setProgress(float progress, String label);
		void setProgressLabel(String label);

		void logDev(String message);
		void logInfo(String message);
		void logWarning(String message);
		void logError(String message);
		void log(LoggerLevel level, String message);
		
		bool isCancelled() const;
		void setError(bool error);
		bool hasError() const;

		size_t getNumMessages() const;
		Vector<std::pair<LoggerLevel, String>> copyMessagesHead(size_t max, std::optional<LoggerLevel> filter = {}) const;
		Vector<std::pair<LoggerLevel, String>> copyMessagesTail(size_t max, std::optional<LoggerLevel> filter = {}) const;

		bool hasPendingTasks() const;
		void addPendingTask(std::unique_ptr<Task> task);
		void onPendingTaskDone(size_t numContinuations);

		Task* getParent() const;

		void setVisible(bool visible);

		virtual std::optional<String> getAction();
		virtual void doAction(TaskSet& taskSet);

	private:
		Vector<std::unique_ptr<Task>> continuations;
		Vector<std::unique_ptr<Task>> pendingTasks;
		Vector<String> toClear;

		mutable std::mutex mutex;
		std::atomic<float> progress;
		String name;
		String progressLabel;

		std::atomic<bool> cancelled;
		std::atomic<bool> hasPendingTasksOnQueue;
		std::atomic<int> pendingTaskCount;

		Task* parent = nullptr;

		const bool isCancellable;
		bool isVisible;
		
		bool error = false;
		std::list<std::pair<LoggerLevel, String>> messageLog;
		std::atomic<size_t> numMessages;

		Vector<String> exclusivityTags;
	};
}
