#pragma once

namespace Halley
{
	class Texture;
	class VideoAPIInternal;
	enum class ShaderParameterType;

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
		MaterialParameter(Material& material, String name, ShaderParameterType type);
		VideoAPIInternal& getAPI();
		unsigned int getAddress();
		void apply();
		void bind();

		std::function<void(MaterialParameter&)> toApply;
		std::function<void(MaterialParameter&)> toBind;

		Material& material;
		String name;
		ShaderParameterType type;
		bool needsTextureUnit = false;
		int textureUnit = -1;
	};

}
