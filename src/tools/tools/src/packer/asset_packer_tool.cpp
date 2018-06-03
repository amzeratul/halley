#include "halley/tools/packer/asset_packer_tool.h"
#include "halley/tools/packer/asset_pack_manifest.h"
#include "halley/file/path.h"
#include "halley/tools/packer/asset_packer.h"
#include "halley/support/logger.h"
#include "halley/file/byte_serializer.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_loader.h"

using namespace Halley;

int AssetPackerTool::run(Vector<std::string> args)
{
	try {
		if (args.size() == 3) {
			const auto manifestPath = Path(args[0]);
			const auto projDir = Path(args[1]);
			const auto halleyDir = Path(args[2]);
						
			// Create project
			ProjectLoader loader(*statics, halleyDir);
			loader.setPlatform(platform);
			auto project = loader.loadProject(projDir);
			project->setAssetPackManifest(Path(args[0]));
			const auto src = Path(args[1]);
			const auto dst = Path(args[2]);

			AssetPacker::pack(*project, {}, {});
			return 0;
		} else {
			Logger::logError("Usage: halley-cmd pack path/to/manifest.yaml projDir halleyDir");
			return 1;
		}
	} catch (std::exception& e) {
		Logger::logException(e);
		return 1;
	} catch (...) {
		Logger::logError("Unknown exception packing files.");
		return 1;
	}
}
