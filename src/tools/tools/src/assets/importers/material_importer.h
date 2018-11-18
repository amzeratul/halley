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
		ImportAssetType getType() const override { return ImportAssetType::Material; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

		MaterialDefinition parseMaterial(Path basePath, gsl::span<const gsl::byte> data, IAssetCollector& collector) const;

	private:
		static void loadPass(MaterialDefinition& material, const ConfigNode& node, IAssetCollector& collector, int passN);
		static void loadUniforms(MaterialDefinition& material, const YAML::Node& topNode);
		static void loadTextures(MaterialDefinition& material, const YAML::Node& topNode);
		static void loadAttributes(MaterialDefinition& material, const YAML::Node& topNode);

		static ShaderParameterType parseParameterType(String rawType);
		static int getAttributeSize(ShaderParameterType type);

		static Bytes loadShader(const String& name, IAssetCollector& collector);
		static Bytes doLoadShader(const String& name, IAssetCollector& collector, std::set<String>& loaded);		
	};
}
