#pragma once

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

	private:
		MaterialParameter(Material& material, String name);
		VideoAPIInternal& getAPI();
		void apply();
		void bind();

		std::function<void()> toApply;
		std::function<void()> toBind;

		Material& material;
		String name;
	};

	class Material : public Resource
	{
		friend class MaterialParameter;

	public:
		Material(std::shared_ptr<Shader> shader, VideoAPI* api);

		void bind();

		MaterialParameter& operator[](String name);
		
		static std::unique_ptr<Material> loadResource(ResourceLoader loader);

	private:
		VideoAPI* api;
		std::shared_ptr<Shader> shader;
		std::vector<MaterialParameter> uniforms;
		bool dirty = false;

		void ensureLoaded();
	};
}
