#pragma once

#include <halley/maths/colour.h>
#include <halley/maths/vector2.h>
#include <halley/maths/vector3.h>
#include <halley/maths/vector4.h>
#include <halley/maths/matrix4.h>
#include <memory>

namespace Halley
{
	enum class ShaderType;
	class Texture;
	class VideoAPIInternal;
	class Material;
	class MaterialPass;
	enum class ShaderParameterType;

	class MaterialTextureParameter
	{
	public:
		MaterialTextureParameter(Material& material, const String& name);
		unsigned int getAddress(int pass, ShaderType stage) const;

	private:
		String name;
		Vector<int> addresses;
	};

	class MaterialParameter
	{
		friend class Material;
		friend class MaterialPass;

	public:
		void operator=(Colour colour);
		void operator=(float p);
		void operator=(Vector2f p);
		void operator=(Vector3f p);
		void operator=(Vector4f p);
		void operator=(int p);
		void operator=(Vector2i p);
		void operator=(Matrix4f m);

		ShaderParameterType getType() const;

	private:
		MaterialParameter(Material& material, const String& name, ShaderParameterType type, int blockNumber, size_t offset);

		void rebind(Material& material);
		
		Material* material;
		ShaderParameterType type;
		String name;
		int blockNumber;
		size_t offset;
	};

}
