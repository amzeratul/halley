#pragma once
#include "halley/tools/assets/iasset_importer.h"

namespace Halley
{
	class Animation;

	class AnimationImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::ANIMATION; }

		std::vector<Path> import(const ImportingAsset& asset, Path dstDir, ProgressReporter reporter, AssetCollector collector) override;

		static void parseAnimation(Animation& animation, gsl::span<const gsl::byte> data);
	};
}
