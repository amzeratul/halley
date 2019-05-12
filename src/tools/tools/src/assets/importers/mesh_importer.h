#pragma once
#include "halley/plugin/iasset_importer.h"
#include <gsl/span>

namespace Halley
{
	class Mesh;

	class MeshImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Mesh; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

	private:
	};
}