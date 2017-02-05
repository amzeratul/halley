#pragma once

#include <memory>
#include "halley/text/halleystring.h"
#include "halley/core/graphics/texture.h"
#include "halley/support/exception.h"
#include "uniform_type.h"

namespace Halley
{
	class Material;
	enum class ShaderParameterType;
	class Painter;
	class MaterialDefinition;
	class MaterialParameter;

	class MaterialConstantBuffer
	{
	public:
		virtual ~MaterialConstantBuffer() {}

		virtual void update(const Material& material) = 0;
	};
	
	class Material
	{
		friend class MaterialParameter;

	public:
		Material(const Material& other);
		explicit Material(std::shared_ptr<const MaterialDefinition> materialDefinition);
		void bind(int pass, Painter& painter);
		static void resetBindCache();

		void setUniform(size_t offset, ShaderParameterType type, void* data);
		void setTexture(int textureUnit, std::shared_ptr<const Texture> texture);

		const MaterialDefinition& getDefinition() const { return *materialDefinition; }

		std::shared_ptr<Material> clone() const;
		
		const std::shared_ptr<const Texture>& getMainTexture() const;
		const std::shared_ptr<const Texture>& getTexture(int textureUnit) const;
		const Bytes& getData() const;
		const Vector<MaterialParameter>& getUniforms() const;
		MaterialConstantBuffer& getConstantBuffer() const;

		template <typename T>
		Material& set(const String& name, const T& value, bool optional = false)
		{
			auto param = getParameter(name);
			if (param) {
				*param = value;
			} else if (!optional) {
				throw Exception("Uniform not available: " + name);
			}
			return *this;
		}

	private:
		bool dirty = false;

		Vector<MaterialParameter> uniforms;
		std::shared_ptr<const MaterialDefinition> materialDefinition;
		std::unique_ptr<MaterialConstantBuffer> constantBuffer;

		Bytes uniformData;
		std::vector<std::shared_ptr<const Texture>> textures;

		void initUniforms();
		MaterialParameter* getParameter(const String& name);
	};
}
