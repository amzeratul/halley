#pragma once
#include "halley/core/graphics/material/material.h"
#include <halley/data_structures/vector.h>

namespace Halley
{
	class ResourceLoader;
	class MaterialParameterBinding;
	class MaterialAttribute;

	enum class ShaderType
	{
		Vertex,
		Pixel,
		Geometry,
		Combined,

		NumOfShaderTypes
	};

	template <>
	struct EnumNames<ShaderType> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"vertex",
				"pixel",
				"geometry",
				"combined"
			}};
		}
	};

	class ShaderDefinition
	{
	public:
		String name;
		std::map<ShaderType, Bytes> shaders;
		Vector<MaterialAttribute> vertexAttributes;
	};

	class ShaderFile : public Resource
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
