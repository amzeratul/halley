#include "codegen_importer.h"
#include "halley/tools/codegen/codegen.h"
#include "halley/tools/assets/import_assets_database.h"

using namespace Halley;

String CodegenImporter::getAssetId(Path file) const
{
	return ":codegen";
}

std::vector<Path> CodegenImporter::import(const ImportAssetsDatabaseEntry& asset, Path dstDir, ProgressReporter reporter)
{
	Codegen codegen;

	std::vector<Path> srcs;
	int n = 0;
	for (auto& f : asset.inputFiles) {
		if (!reporter(lerp(0.0f, 0.25f, float(n) / asset.inputFiles.size()), "Loading sources")) {
			return {};
		}
		srcs.push_back(asset.srcDir / f.first);
		++n;
	}
	codegen.loadSources(srcs, [&](float progress, String label) -> bool {
		return reporter(lerp(0.0f, 0.25f, progress), "Loading " + label);
	});

	if (!reporter(0.25f, "Validating")) {
		return {};
	}
	codegen.validate([&](float progress, String label) -> bool {
		return reporter(lerp(0.25f, 0.5f, progress), "Validating " + label);
	});

	if (!reporter(0.50f, "Processing")) {
		return {};
	}
	codegen.process();

	if (!reporter(0.75f, "Generating code")) {
		return {};
	}
	auto out = codegen.generateCode(dstDir, [&](float progress, String label) -> bool {
		return reporter(lerp(0.75f, 1.0f, progress), "Generating " + label);
	});

	if (!reporter(0.999f, "")) {
		return {};
	}
	
	return out;
}
