#pragma once

#include "prec.h"

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
		void setContinuations(Vector<EditorTaskAnchor>&& tasks);
		void setProgress(float progress, String label = "");
		bool isCancelled() const;

	private:
		Vector<EditorTaskAnchor> continuations;
		mutable std::mutex mutex;
		std::atomic<float> progress = 0;
		String name;
		String progressLabel;

		std::atomic<bool> cancelled;
		const bool isCancellable;
		const bool isVisible;
	};

	class EditorTaskAnchor
	{
	public:
		EditorTaskAnchor(std::unique_ptr<EditorTask> task, float delay = 0);
		EditorTaskAnchor(EditorTaskAnchor&& other);
		~EditorTaskAnchor();

		void update(float time);

		EditorTaskStatus getStatus() const { return status; }
		String getName() const;
		String getProgressLabel() const;
		float getProgress() const { return progress; }

		bool canCancel() const;
		bool isVisible() const;
		void cancel();

		Vector<EditorTaskAnchor> getContinuations();

	private:
		std::unique_ptr<EditorTask> task;
		Future<void> taskFuture;

		EditorTaskStatus status;
		float timeToStart = 0;
		float progress = 0;
		String progressLabel;
	};
}
