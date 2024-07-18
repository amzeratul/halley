#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	enum class ShaderType;
    class MaterialDefinition;

	class ShaderImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Shader; }
		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

		static Bytes convertHLSL(const String& name, ShaderType type, const Bytes& data, const String& dstLanguage);
		static Bytes compileHLSL(const String& name, ShaderType type, const Bytes& data);

        static Bytes compileDXIL(const String& name, ShaderType type, const Bytes& data, const MaterialDefinition& material);
        static Bytes buildRootSignature(const MaterialDefinition& material);

	private:
		static void patchGLSL410(const String& name, ShaderType type, Bytes& data);
		static void patchGLSLCombinedTexSamplers(const String& name, ShaderType type, Bytes& data);
	};
}
