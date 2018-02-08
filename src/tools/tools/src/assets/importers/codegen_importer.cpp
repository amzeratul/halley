#include "codegen_importer.h"
#include "halley/tools/codegen/codegen.h"
#include "halley/tools/assets/import_assets_database.h"

using namespace Halley;

String CodegenImporter::getAssetId(const Path& file, const Maybe<Metadata>& metadata) const
{
	return ":codegen";
}

void CodegenImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Codegen codegen;

	std::vector<std::pair<String, gsl::span<const gsl::byte>>> srcs;
	int n = 0;
	for (auto& f : asset.inputFiles) {
		if (!collector.reportProgress(lerp(0.0f, 0.25f, float(n) / asset.inputFiles.size()), "Loading sources")) {
			return;
		}
		srcs.push_back(std::make_pair(f.name.string(), gsl::as_bytes(gsl::span<const Byte>(f.data))));
		++n;
	}
	codegen.loadSources(srcs, [&](float progress, String label) -> bool {
		return collector.reportProgress(lerp(0.0f, 0.25f, progress), "Loading " + label);
	});

	if (!collector.reportProgress(0.25f, "Validating")) {
		return;
	}
	codegen.validate([&](float progress, String label) -> bool {
		return collector.reportProgress(lerp(0.25f, 0.5f, progress), "Validating " + label);
	});

	if (!collector.reportProgress(0.50f, "Processing")) {
		return;
	}
	codegen.process();

	if (!collector.reportProgress(0.75f, "Generating code")) {
		return;
	}
	codegen.generateCode(collector.getDestinationDirectory(), [&](float progress, String label) -> bool {
		return collector.reportProgress(lerp(0.75f, 1.0f, progress), "Generating " + label);
	});

	if (!collector.reportProgress(0.999f, "")) {
		return;
	}
}
