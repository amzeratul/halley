#pragma once
#include "halley/graphics/material/material.h"
#include <halley/data_structures/vector.h>
#include "shader_type.h"

namespace Halley
{
	class ResourceLoader;
	class MaterialParameterBinding;
	class MaterialAttribute;

	class ShaderDefinition
	{
	public:
		String name;
		std::map<ShaderType, Bytes> shaders;
		Vector<MaterialAttribute> vertexAttributes;
	};

	class ShaderFile final : public Resource
	{
	public:
		std::map<ShaderType, Bytes> shaders;

		static std::unique_ptr<ShaderFile> loadResource(ResourceLoader& loader);
		void reload(Resource&& resource) override;
		constexpr static AssetType getAssetType() { return AssetType::Shader; }

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	class Shader
	{
	public:
		virtual ~Shader() {}
		virtual int getUniformLocation(const String& name, ShaderType stage) = 0;
		virtual int getBlockLocation(const String& name, ShaderType stage) = 0;
	};
}
