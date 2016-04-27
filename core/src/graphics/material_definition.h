#pragma once
#include "blend.h"

namespace YAML
{
	class Node;
}

namespace Halley
{
	class MaterialPass;
	class ResourceLoader;
	class Shader;
	class VideoAPI;
	class Painter;

	enum class ShaderParameterType
	{
		Float,
		Float2,
		Float3,
		Float4,
		Int,
		Int2,
		Int3,
		Int4,
		Matrix2,
		Matrix3,
		Matrix4,
		Texture2D
	};

	class MaterialAttribute
	{
	public:
		String name;
		ShaderParameterType type;
		int location;
		int offset;

		MaterialAttribute() {}
		MaterialAttribute(String name, ShaderParameterType type, int location, int offset = 0)
			: name(name)
			, type(type)
			, location(location)
			, offset(offset)
		{}
	};

	class MaterialDefinition : public Resource
	{
		friend class MaterialParameter;

	public:
		explicit MaterialDefinition(ResourceLoader& loader);

		void bind(int pass, Painter& painter);
		int getNumPasses() const;
		MaterialPass& getPass(int n);

		int getVertexStride() const { return vertexStride; }
		const std::vector<MaterialAttribute>& getAttributes() const { return attributes; }
		const std::vector<MaterialAttribute>& getUniforms() const { return uniforms; }

		static std::unique_ptr<MaterialDefinition> loadResource(ResourceLoader& loader);

	private:
		VideoAPI* api;
		std::vector<MaterialPass> passes;
		std::vector<MaterialAttribute> uniforms;
		std::vector<MaterialAttribute> attributes;
		int vertexStride;
		bool dirty = false;
		String name;

		void loadPass(YAML::Node node, std::function<String(String)> retriever);
		void loadUniforms(YAML::Node node);
		void loadAttributes(YAML::Node node);

		static ShaderParameterType parseParameterType(String string);
		static int getAttributeSize(ShaderParameterType type);
	};

	class MaterialPass
	{
		friend class Material;

	public:
		MaterialPass(std::shared_ptr<Shader> shader, BlendType blend);

		void bind(Painter& painter);

		BlendType getBlend() const { return blend; }
		Shader& getShader() const { return *shader; }

	private:
		std::shared_ptr<Shader> shader;
		BlendType blend;
	};
}
