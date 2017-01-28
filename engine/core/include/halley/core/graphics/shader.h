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
		Geometry
	};

	template <>
	struct EnumNames<ShaderType> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"vertex",
				"pixel",
				"geometry"
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

	class Shader
	{
	public:
		virtual ~Shader() {}
		virtual void bind() = 0;
		virtual unsigned int getUniformLocation(String name) = 0;
	};
}
