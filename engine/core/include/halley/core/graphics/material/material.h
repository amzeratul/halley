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

		const MaterialDefinition& getDefinition() const { return *materialDefinition; }

		std::shared_ptr<Material> clone() const;
		
		const std::shared_ptr<const Texture>& getMainTexture() const;
		const std::shared_ptr<const Texture>& getTexture(int textureUnit) const;
		const Bytes& getData() const;
		const Vector<MaterialParameter>& getUniforms() const;
		const Vector<MaterialParameter>& getTextureUniforms() const;
		MaterialConstantBuffer& getConstantBuffer() const;

		Material& set(const String& name, const std::shared_ptr<const Texture>& texture);

		template <typename T>
		Material& set(const String& name, const T& value)
		{
			getParameter(name) = value;
			return *this;
		}

	private:
		bool dirty = false;

		std::shared_ptr<const MaterialDefinition> materialDefinition;
		
		Vector<MaterialParameter> uniforms;
		std::unique_ptr<MaterialConstantBuffer> constantBuffer;
		Bytes uniformData;

		Vector<MaterialParameter> textureUniforms;
		std::vector<std::shared_ptr<const Texture>> textures;

		void initUniforms();
		MaterialParameter& getParameter(const String& name);

		void setUniform(size_t offset, ShaderParameterType type, void* data);
	};
}
