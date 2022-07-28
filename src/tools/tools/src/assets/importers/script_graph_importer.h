#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class ScriptGraph;

	class ScriptGraphImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::ScriptGraph; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

	private:
		void loadScriptDependencies(const String& assetId, ScriptGraph& graph, IAssetCollector& collector) const;
		ScriptGraph loadScript(const String& assetId, const Bytes& bytes, IAssetCollector& collector) const;
	};
}
