#include "halley/tools/packer/asset_packer_task.h"
#include "halley/tools/project/project.h"
#include "halley/support/logger.h"

using namespace Halley;

AssetPackerTask::AssetPackerTask(Project& project, Maybe<std::set<String>> assetsToPack, std::vector<String> deletedAssets)
	: EditorTask("Pack Assets", true, true)
	, project(project)
	, assetsToPack(std::move(assetsToPack))
	, deletedAssets(std::move(deletedAssets))
{
}

void AssetPackerTask::run()
{
	Logger::logInfo("Packing assets (" + toString(assetsToPack->size()) + " modified).");
	AssetPacker::pack(project, assetsToPack, deletedAssets);
	Logger::logInfo("Done packing assets");

	if (!isCancelled()) {
		setProgress(1.0f, "");

		if (assetsToPack) {
			project.reloadAssets(assetsToPack.get());
		}
	}
}
