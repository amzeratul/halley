#pragma once

#include <halley/maths/colour.h>
#include <halley/maths/vector2.h>
#include <halley/maths/vector3.h>
#include <halley/maths/vector4.h>
#include <halley/maths/matrix4.h>
#include <memory>

namespace Halley
{
	enum class TextureSamplerType : uint8_t;
	enum class ShaderType;
	class Texture;
	class VideoAPIInternal;
	class Material;
	class MaterialPass;
	enum class ShaderParameterType : uint8_t;

	class MaterialParameter
	{
		friend class Material;
		friend class MaterialPass;

	public:
		bool set(Colour colour);
		bool set(float p);
		bool set(Vector2f p);
		bool set(Vector3f p);
		bool set(Vector4f p);
		bool set(int p);
		bool set(Vector2i p);
		bool set(Vector3i p);
		bool set(Vector4i p);
		bool set(uint32_t p);
		bool set(const Matrix4f& m);

	private:
		MaterialParameter(Material& material, ShaderParameterType type, uint16_t blockNumber, uint32_t offset);

		Material* material;
		uint32_t offset;
		uint16_t blockNumber;
		ShaderParameterType type;
	};

}
