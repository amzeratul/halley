#pragma once

#include <memory>
#include "halley/text/halleystring.h"

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

		MaterialParameter& operator[](String name);

		const MaterialDefinition& getDefinition() const { return *materialDefinition; }

		std::shared_ptr<Material> clone() const;

	private:
		bool dirty = false;

		Vector<MaterialParameter> uniforms;
		std::shared_ptr<const MaterialDefinition> materialDefinition;

		void updateUniforms();
	};
}
