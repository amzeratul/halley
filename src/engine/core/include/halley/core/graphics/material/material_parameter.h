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
		MaterialParameter& operator=(Colour colour);
		MaterialParameter& operator=(float p);
		MaterialParameter& operator=(Vector2f p);
		MaterialParameter& operator=(Vector3f p);
		MaterialParameter& operator=(Vector4f p);
		MaterialParameter& operator=(int p);
		MaterialParameter& operator=(Vector2i p);
		MaterialParameter& operator=(const Matrix4f& m);

		ShaderParameterType getType() const;

	private:
		MaterialParameter(Material& material, String name, ShaderParameterType type, int blockNumber, size_t offset);

		void rebind(Material& material) noexcept;
		
		Material* material;
		String name;
		size_t offset;
		ShaderParameterType type;
		int blockNumber;
	};

}
