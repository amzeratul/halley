#pragma once

#include "halley/data_structures/vector.h"
#include "halley/text/halleystring.h"
#include "halley/concurrency/future.h"

namespace Halley
{
	enum class LoggerLevel;

	enum class EditorTaskStatus
	{
		WaitingToStart,
		Started,
		Done
	};

	class EditorTask;

	class EditorTaskAnchor
	{
	public:
		EditorTaskAnchor(std::unique_ptr<EditorTask> task, float delay = 0);
		EditorTaskAnchor(EditorTaskAnchor&& other) noexcept;
		EditorTaskAnchor(const EditorTaskAnchor& other) = delete;
		~EditorTaskAnchor();

		EditorTaskAnchor& operator=(EditorTaskAnchor&& other) noexcept;
		EditorTaskAnchor& operator=(const EditorTaskAnchor& other) = delete;

		void terminate();
		void update(float time);

		EditorTaskStatus getStatus() const;
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
		std::vector<std::pair<LoggerLevel, String>> copyMessagesHead(size_t max, std::optional<LoggerLevel> filter = {}) const;
		std::vector<std::pair<LoggerLevel, String>> copyMessagesTail(size_t max, std::optional<LoggerLevel> filter = {}) const;

		Vector<std::unique_ptr<EditorTask>> getContinuations();
		Vector<std::unique_ptr<EditorTask>> getPendingTasks();

	private:
		std::unique_ptr<EditorTask> task;
		Future<void> taskFuture;

		EditorTaskStatus status;
		float timeToStart = 0;
		float progress = 0;
		bool terminated = false;
		bool error = false;
		String progressLabel;

		int id = 0;
	};
}
