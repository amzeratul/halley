#pragma once

#include "../../core/src/graphics/shader.h"

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
		void setAttributes(const std::vector<MaterialAttribute>& attributes) override;

	private:
		unsigned int id = 0;
		bool ready = false;
		std::vector<String> vertexSources;
		std::vector<String> pixelSources;
		std::vector<String> geometrySources;
		std::vector<unsigned int> shaders;

		std::map<String, unsigned int> attributeLocations;
		std::map<String, unsigned int> uniformLocations;
	};
}
