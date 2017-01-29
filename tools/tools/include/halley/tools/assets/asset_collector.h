#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley {
	class AssetCollector final : public IAssetCollector
	{
	public:
		using ProgressReporter = std::function<bool(float, const String&)>;

		AssetCollector(const Path& dstDir, const std::vector<Path>& assetsSrc, ProgressReporter reporter);

		void output(const Path& path, const Bytes& data, Maybe<Metadata> metadata) override;
		void output(const Path& path, gsl::span<const gsl::byte> data, Maybe<Metadata> metadata) override;

		void addAdditionalAsset(ImportingAsset&& asset) override;
		bool reportProgress(float progress, const String& label) override;
		const Path& getDestinationDirectory() override;
		Bytes readAdditionalFile(const Path& filePath) const override;

		const std::vector<Path>& getOutFiles() const;
		std::vector<ImportingAsset> collectAdditionalAssets();
		
	private:
		Path dstDir;
		std::vector<Path> assetsSrc;
		ProgressReporter reporter;
		std::vector<Path> outFiles;
		std::vector<ImportingAsset> additionalAssets;
	};
}
