#pragma once

#include <memory>
#include "halley/text/halleystring.h"
#include "halley/core/graphics/texture.h"

namespace Halley
{
	class Painter;
	class MaterialDefinition;
	class MaterialParameter;
	
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

		Material& set(const String& name, const std::shared_ptr<const Texture>& texture);

		template <typename T>
		Material& set(const String& name, const T& value)
		{
			getParameter(name) = value;
			return *this;
		}

	private:
		bool dirty = false;

		Vector<MaterialParameter> uniforms;
		std::shared_ptr<const MaterialDefinition> materialDefinition;
		std::shared_ptr<const Texture> mainTexture;

		void updateUniforms();
		void setMainTexture(const std::shared_ptr<const Texture>& tex);
		MaterialParameter& getParameter(const String& name);
	};
}
