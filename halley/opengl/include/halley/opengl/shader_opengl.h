#pragma once

#include "halley/core/graphics/shader.h"
#include <halley/data_structures/hash_map.h>

namespace Halley
{
	class ShaderOpenGL final : public Shader
	{
	public:
		ShaderOpenGL(String name);
		~ShaderOpenGL();

		void bind() override;
		void unbind();
		void compile() override;
		void destroy();

		void addVertexSource(String src) override;
		void addGeometrySource(String src) override;
		void addPixelSource(String src) override;
		
		unsigned getUniformLocation(String name) override;
		unsigned getAttributeLocation(String name) override;
		void setAttributes(const Vector<MaterialAttribute>& attributes) override;

	private:
		unsigned int id = 0;
		bool ready = false;
		Vector<String> vertexSources;
		Vector<String> pixelSources;
		Vector<String> geometrySources;
		Vector<unsigned int> shaders;

		HashMap<String, unsigned int> attributeLocations;
		HashMap<String, unsigned int> uniformLocations;
	};
}
