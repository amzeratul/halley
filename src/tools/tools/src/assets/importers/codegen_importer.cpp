#include "codegen_importer.h"
#include "halley/tools/codegen/codegen.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/ecs/ecs_data.h"

using namespace Halley;

String CodegenImporter::getAssetId(const Path& file, const Maybe<Metadata>& metadata) const
{
	return ":codegen";
}

void CodegenImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	std::vector<CodegenSourceInfo> srcs;
	int n = 0;
	for (auto& f : asset.inputFiles) {
		if (!collector.reportProgress(lerp(0.0f, 0.25f, float(n) / asset.inputFiles.size()), "Loading sources")) {
			return;
		}
		srcs.push_back(CodegenSourceInfo{ f.name.string(), gsl::as_bytes(gsl::span<const Byte>(f.data)), !f.metadata.getBool("skipGen", false) });
		++n;
	}
	ECSData data;
	data.loadSources(srcs);

	if (!collector.reportProgress(0.5f, "Generating code")) {
		return;
	}
	Codegen::generateCode(data, collector.getDestinationDirectory());

	if (!collector.reportProgress(0.999f, "")) {
		return;
	}
}
