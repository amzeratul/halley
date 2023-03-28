#include "halley/tools/packer/asset_packer_task.h"
#include "halley/tools/project/project.h"
#include "halley/support/logger.h"
#include "halley/concurrency/concurrent.h"
#include "halley/concurrency/task_set.h"

using namespace Halley;

AssetPackerTask::AssetPackerTask(Project& project, std::optional<std::set<String>> assetsToPack, Vector<String> deletedAssets)
	: Task("Packing assets", true, true, { "assets" })
	, project(project)
	, assetsToPack(std::move(assetsToPack))
	, deletedAssets(std::move(deletedAssets))
{
}

void AssetPackerTask::run()
{
	try {
		Logger::logInfo("Packing assets (" + toString(assetsToPack->size()) + " modified).");
		auto packs = AssetPacker::pack(project, assetsToPack, deletedAssets, [=](float p, const String& s) { setProgress(p * 0.95f, s); });
		Logger::logInfo("Done packing assets");

		if (!isCancelled()) {
			setProgress(1.0f, "");

			if (assetsToPack) {
				Concurrent::execute(Executors::getMainUpdateThread(), [project = &project, assets = std::move(assetsToPack), packs = std::move(packs)]() {
					project->reloadAssets(assets.value(), packs, true);
				});
			}
		}
	} catch (const std::exception& e) {
		logError("Exception packing assets: " + String(e.what()));
	} catch (...) {
		logError("Unknown exception.");
	}
}

std::optional<String> AssetPackerTask::getAction()
{
	if (hasError()) {
		return "Repack";
	} else {
		return std::nullopt;
	}
}

void AssetPackerTask::doAction(TaskSet& taskSet)
{
	auto task = std::make_unique<AssetPackerTask>(project, std::move(assetsToPack), std::move(deletedAssets));
	Concurrent::execute(Executors::getImmediate(), [&]()
	{
		return std::move(task);
	}).then(Executors::getMainUpdateThread(), [&taskSet] (std::unique_ptr<Task> task)
	{
		taskSet.addTask(std::move(task));
	});
}
