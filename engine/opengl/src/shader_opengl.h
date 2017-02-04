#pragma once

#include "halley/core/graphics/shader.h"
#include <halley/data_structures/hash_map.h>

namespace Halley
{
	class ShaderOpenGL final : public Shader
	{
	public:
		explicit ShaderOpenGL(const ShaderDefinition& definition);
		~ShaderOpenGL();

		void bind();
		void unbind();
		void destroy();

		unsigned getUniformLocation(String name) override;
		unsigned getAttributeLocation(String name);

	private:
		unsigned int id = 0;
		bool ready = false;
		Vector<Bytes> vertexSources;
		Vector<Bytes> pixelSources;
		Vector<Bytes> geometrySources;
		Vector<unsigned int> shaders;

		HashMap<String, unsigned int> attributeLocations;
		HashMap<String, unsigned int> uniformLocations;

		String name;

		void loadShaders(const std::map<ShaderType, Bytes>& shaders);
		void compile();
		void setAttributes(const Vector<MaterialAttribute>& attributes);
	};
}
