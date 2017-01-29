#pragma once
#include "halley/plugin/iasset_importer.h"
#include <yaml-cpp/node/node.h>
#include "halley/core/graphics/material/material_definition.h"
#include <gsl/span>

namespace Halley
{
	class MaterialDefinition;

	class MaterialImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::Material; }

		std::vector<Path> import(const ImportingAsset& asset, const Path& dstDir, ProgressReporter reporter, AssetCollector collector) override;

		MaterialDefinition parseMaterial(Path basePath, gsl::span<const gsl::byte> data) const;

	private:
		static void loadPass(MaterialDefinition& material, const YAML::Node& node);
		static void loadUniforms(MaterialDefinition& material, const YAML::Node& topNode);
		static void loadAttributes(MaterialDefinition& material, const YAML::Node& topNode);

		static ShaderParameterType parseParameterType(String rawType);
		static int getAttributeSize(ShaderParameterType type);
	};
}
