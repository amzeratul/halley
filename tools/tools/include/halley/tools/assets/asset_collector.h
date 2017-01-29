#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley {
	class AssetCollector final : public IAssetCollector
	{
	public:
		using ProgressReporter = std::function<bool(float, const String&)>;

		AssetCollector(const ImportingAsset& asset, const Path& dstDir, const std::vector<Path>& assetsSrc, ProgressReporter reporter);

		void output(AssetType type, const Bytes& data, Maybe<Metadata> metadata) override;
		void output(AssetType type, gsl::span<const gsl::byte> data, Maybe<Metadata> metadata) override;

		void addAdditionalAsset(ImportingAsset&& asset) override;
		bool reportProgress(float progress, const String& label) override;
		const Path& getDestinationDirectory() override;
		Bytes readAdditionalFile(const Path& filePath) const override;

		std::vector<AssetResource> collectAssets();
		std::vector<ImportingAsset> collectAdditionalAssets();
		
	private:
		const ImportingAsset& asset;
		Path dstDir;
		std::vector<Path> assetsSrc;
		ProgressReporter reporter;
		std::vector<AssetResource> assets;
		std::vector<ImportingAsset> additionalAssets;
	};
}
