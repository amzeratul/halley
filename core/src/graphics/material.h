#pragma once
#include "../../../opengl/src/gl_utils.h"

namespace Halley
{
	class VideoAPI;
	class ResourceLoader;
	class Shader;
	class Texture;

	class MaterialParameter
	{
		friend class Material;

	public:
		void operator=(std::shared_ptr<Texture> texture);
		void operator=(Colour colour);
		void operator=(float p);
		void operator=(Vector2f p);
		void operator=(int p);
		void operator=(Vector2i p);
		void operator=(Matrix4f m);

	private:
		MaterialParameter(Material& material, String name);
		VideoAPIInternal& getAPI();
		unsigned int getAddress();
		void apply();
		void bind();

		std::function<void(MaterialParameter&)> toApply;
		std::function<void(MaterialParameter&)> toBind;

		Material& material;
		String name;
		bool needsTextureUnit = false;
		int textureUnit = -1;
	};

	class Material : public Resource
	{
		friend class MaterialParameter;

	public:
		Material(std::shared_ptr<Shader> shader, VideoAPI* api);

		void bind();
		Shader& getShader() const;

		MaterialParameter& operator[](String name);
		Blend::Type getBlend() const;
		static std::unique_ptr<Material> loadResource(ResourceLoader loader);

	private:
		VideoAPI* api;
		std::shared_ptr<Shader> shader;
		std::vector<MaterialParameter> uniforms;
		bool dirty = false;

		void ensureLoaded();
	};
}
