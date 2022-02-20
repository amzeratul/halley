#pragma once
#include "halley/concurrency/task.h"
#include "asset_packer.h"
#include "halley/data_structures/maybe.h"

namespace Halley
{
	class Project;
	
	class AssetPackerTask : public Task
	{
	public:
		explicit AssetPackerTask(Project& project, std::optional<std::set<String>> assetsToPack, Vector<String> deletedAssets);

	protected:
		void run() override;

	private:
		Project& project;
		std::optional<std::set<String>> assetsToPack;
		Vector<String> deletedAssets;
	};
}
