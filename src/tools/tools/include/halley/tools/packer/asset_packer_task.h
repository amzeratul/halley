#pragma once
#include "halley/tools/tasks/editor_task.h"
#include "halley/file/path.h"
#include <vector>
#include "asset_packer.h"

namespace Halley
{
	class Project;
	
	class AssetPackerTask : public EditorTask
	{
	public:
		explicit AssetPackerTask(Project& project);

	protected:
		void run() override;

	private:
		Project& project;
	};
}
