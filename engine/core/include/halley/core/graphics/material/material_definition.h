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
		MaterialDefinition();
		explicit MaterialDefinition(ResourceLoader& loader);

		void bind(int pass, Painter& painter);
		int getNumPasses() const;
		MaterialPass& getPass(int n);

		int getVertexStride() const { return vertexStride; }
		int getVertexPosOffset() const { return vertexPosOffset; }
		const Vector<MaterialAttribute>& getAttributes() const { return attributes; }
		const Vector<MaterialAttribute>& getUniforms() const { return uniforms; }

		static std::unique_ptr<MaterialDefinition> loadResource(ResourceLoader& loader);

		void loadShader(VideoAPI* api);

	private:
		VideoAPI* api = nullptr;

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
		MaterialPass(BlendType blend, String vertexSrc, String geometrySrc, String pixelSrc);

		void bind(Painter& painter) const;

		BlendType getBlend() const { return blend; }
		Shader& getShader() const { return *shader; }

		void createShader(VideoAPI* api, String name, const Vector<MaterialAttribute>& attributes);

	private:
		std::shared_ptr<Shader> shader;
		BlendType blend;
		
		String vertexSrc;
		String geometrySrc;
		String pixelSrc;
	};
}
