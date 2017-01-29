#pragma once
#include "halley/plugin/iasset_importer.h"
#include <gsl/gsl>

namespace Halley
{
	class Animation;

	class AnimationImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::Animation; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

		static void parseAnimation(Animation& animation, gsl::span<const gsl::byte> data);
	};
}
