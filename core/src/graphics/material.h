#pragma once

namespace Halley
{
	class ResourceLoader;
	class Shader;
	class Texture;

	class Material : public Resource
	{
	public:
		Material(std::shared_ptr<Shader> shader);

		void setTexture(String name, std::shared_ptr<Texture> texture);
		
		static std::unique_ptr<Material> loadResource(ResourceLoader loader);
	};
}
