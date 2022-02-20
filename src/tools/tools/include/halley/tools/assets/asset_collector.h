#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley {
	class AssetCollector final : public IAssetCollector
	{
	public:
		using ProgressReporter = std::function<bool(float, const String&)>;

		AssetCollector(const ImportingAsset& asset, const Path& dstDir, const Vector<Path>& assetsSrc, ProgressReporter reporter);

		void output(const String& name, AssetType type, const Bytes& data, std::optional<Metadata> metadata, const String& platform, const Path& primaryInputFile) override;

		void addAdditionalAsset(ImportingAsset&& asset) override;
		bool reportProgress(float progress, const String& label) override;
		const Path& getDestinationDirectory() override;
		Bytes readAdditionalFile(const Path& filePath) override;

		Vector<ImportingAsset> collectAdditionalAssets();
		Vector<std::pair<Path, Bytes>> collectOutFiles();
		const Vector<AssetResource>& getAssets() const;
		const Vector<TimestampedPath>& getAdditionalInputs() const;
		
	private:
		const ImportingAsset& asset;
		Path dstDir;
		Vector<Path> assetsSrc;
		ProgressReporter reporter;

		Vector<AssetResource> assets;
		Vector<ImportingAsset> additionalAssets;
		Vector<TimestampedPath> additionalInputs;
		Vector<std::pair<Path, Bytes>> outFiles;
	};
}
