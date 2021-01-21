#pragma once
#include "halley/tools/cli_tool.h"
#include "halley/concurrency/task_set.h"

namespace Halley
{
	class ImportTool : public CommandLineTool, public TaskSetListener
	{
	public:
		int run(Vector<std::string> args) override;
		void onTaskAdded(const std::shared_ptr<TaskAnchor>& task) override;
		void onTaskTerminated(const std::shared_ptr<TaskAnchor>& task) override;
		void onTaskError(const std::shared_ptr<TaskAnchor>& task) override;

	private:
		bool hasError = false;
	};
}
