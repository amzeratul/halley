#pragma once
#include "halley/tools/assets/iasset_importer.h"

namespace Halley
{
	class AudioImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::Audio; }

		std::vector<Path> import(const ImportingAsset& asset, Path dstDir, ProgressReporter reporter, AssetCollector collector) override;

	private:
		Bytes encodeVorbis(int channels, int sampleRate, gsl::span<const std::vector<float>> src);
		static std::vector<float> resampleChannel(int from, int to, gsl::span<const float> src);
	};
}
