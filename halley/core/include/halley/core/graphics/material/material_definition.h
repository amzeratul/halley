#pragma once
#include "halley/core/graphics/blend.h"
#include "halley/resources/resource.h"
#include <functional>

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
		Texture2D,
		Invalid
	};

	class MaterialAttribute
	{
	public:
		String name;
		ShaderParameterType type;
		int location;
		int offset;

		MaterialAttribute()
			: type(ShaderParameterType::Invalid)
			, location(-1)
			, offset(-1)
		{}

		MaterialAttribute(String name, ShaderParameterType type, int location, int offset = 0)
			: name(name)
			, type(type)
			, location(location)
			, offset(offset)
		{}
	};

	class MaterialDefinition final : public Resource
	{
		friend class MaterialParameter;

	public:
		explicit MaterialDefinition(ResourceLoader& loader);

		void bind(int pass, Painter& painter);
		int getNumPasses() const;
		MaterialPass& getPass(int n);

		int getVertexStride() const { return vertexStride; }
		int getVertexPosOffset() const { return vertexPosOffset; }
		const Vector<MaterialAttribute>& getAttributes() const { return attributes; }
		const Vector<MaterialAttribute>& getUniforms() const { return uniforms; }

		static std::unique_ptr<MaterialDefinition> loadResource(ResourceLoader& loader);

	private:
		VideoAPI* api;
		Vector<MaterialPass> passes;
		Vector<MaterialAttribute> uniforms;
		Vector<MaterialAttribute> attributes;
		int vertexStride = 0;
		int vertexPosOffset = 0;
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
