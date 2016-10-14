#pragma once

#include <halley/maths/colour.h>
#include <halley/maths/vector2.h>
#include <halley/maths/vector3.h>
#include <halley/maths/vector4.h>
#include <halley/maths/matrix4.h>
#include <memory>

namespace Halley
{
	class Texture;
	class VideoAPIInternal;
	class Material;
	enum class ShaderParameterType;

	class MaterialParameter
	{
		friend class Material;

	public:
		void operator=(std::shared_ptr<const Texture> texture);
		void operator=(Colour colour);
		void operator=(float p);
		void operator=(Vector2f p);
		void operator=(Vector3f p);
		void operator=(Vector4f p);
		void operator=(int p);
		void operator=(Vector2i p);
		void operator=(Matrix4f m);

	private:
		MaterialParameter(Material& material, String name, ShaderParameterType type);
		VideoAPIInternal& getAPI() const;

		void updateAddresses();
		unsigned int getAddress(int pass);
		void bind(int pass);

		std::function<void(int)> toBind;
		std::function<void(int, void*)> bindFunc;

		Vector<int> addresses;
		Material& material;
		String name;
		ShaderParameterType type;
		bool needsTextureUnit = false;
		int textureUnit = -1;
	};

}
