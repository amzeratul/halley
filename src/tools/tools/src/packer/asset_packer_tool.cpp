#include "halley/tools/packer/asset_packer_tool.h"
#include "halley/tools/packer/asset_pack_manifest.h"
#include "halley/file/path.h"
#include "halley/tools/packer/asset_packer.h"
#include "halley/support/logger.h"
#include "halley/file/byte_serializer.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file_formats/config_file.h"
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/node/node.h>
#include "../assets/importers/config_importer.h"
#include "halley/tools/project/project.h"

using namespace Halley;

int AssetPackerTool::run(Vector<std::string> args)
{
	try {
		if (args.size() == 3) {
			const auto manifestPath = Path(args[0]);
			const auto projDir = Path(args[1]);
			const auto halleyDir = Path(args[2]);

			// Load manifest
			const auto data = FileSystem::readFile(args[0]);
			ConfigFile config;
			String strData(reinterpret_cast<const char*>(data.data()), data.size());
			YAML::Node root = YAML::Load(strData.cppStr());
			config.getRoot() = ConfigImporter::parseYAMLNode(root);
			const auto manifest = AssetPackManifest(config);
			
			// Create project
			Project project(*statics, platform, projDir, halleyDir);
			const auto src = Path(args[1]);
			const auto dst = Path(args[2]);

			AssetPacker::pack(manifest, project.getUnpackedAssetsPath(), project.getPackedAssetsPath());
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
