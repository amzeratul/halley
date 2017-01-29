#pragma once
#include "halley/plugin/iasset_importer.h"
#include <gsl/gsl>

namespace Halley
{
	class Animation;

	class AnimationImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Animation; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

		static void parseAnimation(Animation& animation, gsl::span<const gsl::byte> data);
	};
}
