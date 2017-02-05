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
	class MaterialPass;
	enum class ShaderParameterType;

	class MaterialParameter
	{
		friend class Material;
		friend class MaterialPass;

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

		ShaderParameterType getType() const;
		const void* getData() const;
		unsigned int getAddress(int pass) const;
		const String& getName() const;

	private:
		MaterialParameter(Material& material, String name, ShaderParameterType type, size_t offset);

		void init();
		
		Vector<int> addresses;
		Material& material;
		String name;
		ShaderParameterType type;
		size_t offset;
		int textureUnit = -1;
	};

}
