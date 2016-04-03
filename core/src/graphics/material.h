#pragma once

#include "material_parameter_binding.h"

namespace Halley
{
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

		Material& material;
		String name;
	};

	class Material : public Resource
	{
		friend class MaterialParameter;

	public:
		Material(std::shared_ptr<Shader> shader);

		void bind();

		MaterialParameter operator[](String name);
		
		static std::unique_ptr<Material> loadResource(ResourceLoader loader);

	private:
		std::shared_ptr<Shader> shader;
		std::vector<MaterialParameterBinding> uniforms;

		void ensureLoaded();
	};
}
