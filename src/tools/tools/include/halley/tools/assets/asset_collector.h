#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley {
	class AssetCollector final : public IAssetCollector
	{
	public:
		using ProgressReporter = std::function<bool(float, const String&)>;

		AssetCollector(const ImportingAsset& asset, const Path& dstDir, const std::vector<Path>& assetsSrc, ProgressReporter reporter);

		void output(const String& name, AssetType type, const Bytes& data, Maybe<Metadata> metadata) override;

		void addAdditionalAsset(ImportingAsset&& asset) override;
		bool reportProgress(float progress, const String& label) override;
		const Path& getDestinationDirectory() override;
		Bytes readAdditionalFile(const Path& filePath) override;

		std::vector<ImportingAsset> collectAdditionalAssets();
		std::vector<std::pair<Path, Bytes>> collectOutFiles();
		const std::vector<AssetResource>& getAssets() const;
		const std::vector<TimestampedPath>& getAdditionalInputs() const;
		
	private:
		const ImportingAsset& asset;
		Path dstDir;
		std::vector<Path> assetsSrc;
		ProgressReporter reporter;

		std::vector<AssetResource> assets;
		std::vector<ImportingAsset> additionalAssets;
		std::vector<TimestampedPath> additionalInputs;
		std::vector<std::pair<Path, Bytes>> outFiles;
	};
}
