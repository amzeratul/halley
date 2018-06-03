#pragma once
#include "halley/tools/tasks/editor_task.h"
#include "asset_packer.h"
#include "halley/data_structures/maybe.h"

namespace Halley
{
	class Project;
	
	class AssetPackerTask : public EditorTask
	{
	public:
		explicit AssetPackerTask(Project& project, Maybe<std::set<String>> assetsToPack, std::vector<String> deletedAssets);

	protected:
		void run() override;

	private:
		Project& project;
		Maybe<std::set<String>> assetsToPack;
		std::vector<String> deletedAssets;
	};
}
