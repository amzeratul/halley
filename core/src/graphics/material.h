#pragma once
#include "../../../opengl/src/gl_utils.h"

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
	};

	class Material : public Resource
	{
		friend class MaterialParameter;

	public:
		explicit Material(ResourceLoader& loader);

		void bind(size_t pass);

		size_t getNumPasses() const;
		MaterialPass& getPass(size_t n);

		int getVertexStride() const { return vertexStride; }
		const std::vector<MaterialAttribute>& getAttributes() const { return attributes; }

		MaterialParameter& operator[](String name);
		static std::unique_ptr<Material> loadResource(ResourceLoader& loader);

	private:
		VideoAPI* api;
		std::vector<MaterialPass> passes;
		std::vector<MaterialParameter> uniforms;
		std::vector<MaterialAttribute> attributes;
		int vertexStride;
		bool dirty = false;
		String name;

		void ensureLoaded();
		void updateUniforms();

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
		MaterialPass(std::shared_ptr<Shader> shader, Blend::Type blend);

		void bind();

		Blend::Type getBlend() const { return blend; }
		Shader& getShader() const { return *shader; }

	private:
		std::shared_ptr<Shader> shader;
		Blend::Type blend;
	};
}
