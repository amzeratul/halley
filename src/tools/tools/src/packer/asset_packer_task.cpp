#include "halley/tools/packer/asset_packer_task.h"
#include "halley/tools/project/project.h"
#include "halley/core/devcon/devcon_server.h"

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
	AssetPacker::pack(project, assetsToPack, deletedAssets);

	if (!isCancelled()) {
		setProgress(1.0f, "");

		if (project.getDevConServer() && assetsToPack) {
			project.getDevConServer()->reloadAssets(assetsToPack.get());
		}
	}
}
