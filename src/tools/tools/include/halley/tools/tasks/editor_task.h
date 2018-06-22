#pragma once

#include <mutex>
#include <atomic>

#include "halley/data_structures/vector.h"
#include "halley/text/halleystring.h"
#include "halley/concurrency/future.h"

namespace Halley
{
	enum class EditorTaskStatus
	{
		WaitingToStart,
		Started,
		Done
	};

	class EditorTaskAnchor;

	class EditorTask
	{
		friend class EditorTaskAnchor;

	public:
		virtual ~EditorTask() {}

	protected:
		EditorTask(String name, bool isCancellable, bool isVisible);

		virtual void run() = 0;
		void addContinuation(EditorTaskAnchor&& task);
		void setContinuations(Vector<EditorTaskAnchor>&& tasks);
		void setProgress(float progress, String label = "");
		void addError(const String& message);
		
		bool isCancelled() const;
		bool hasError() const;
		const String& getError() const;

		bool hasPendingTasks() const;
		void addPendingTask(EditorTaskAnchor&& task);
		void onPendingTaskDone(const EditorTaskAnchor& editorTaskAnchor);

	private:
		Vector<EditorTaskAnchor> continuations;
		Vector<EditorTaskAnchor> pendingTasks;

		mutable std::mutex mutex;
		std::atomic<float> progress;
		String name;
		String progressLabel;

		std::atomic<bool> cancelled;
		std::atomic<bool> hasPendingTasksOnQueue;
		std::atomic<int> pendingTaskCount;

		const bool isCancellable;
		const bool isVisible;
		
		bool error = false;
		String errorMsg;
	};

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
		const String& getError() const;

		Vector<EditorTaskAnchor> getContinuations();
		Vector<EditorTaskAnchor> getPendingTasks();
		void setParent(EditorTask& editorTask);

	private:
		std::unique_ptr<EditorTask> task;
		Future<void> taskFuture;

		EditorTask* parent = nullptr;

		EditorTaskStatus status;
		float timeToStart = 0;
		float progress = 0;
		bool terminated = false;
		bool error = false;
		String errorMsg;
		String progressLabel;

		int id = 0;
	};
}
