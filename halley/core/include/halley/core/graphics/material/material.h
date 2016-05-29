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
		explicit Material(std::shared_ptr<MaterialDefinition> materialDefinition);
		void bind(int pass, Painter& painter);

		MaterialParameter& operator[](String name);

		MaterialDefinition& getDefinition() { return *materialDefinition; }
		const MaterialDefinition& getDefinition() const { return *materialDefinition; }

		std::shared_ptr<Material> clone() const;

	private:
		bool dirty = false;

		std::vector<MaterialParameter> uniforms;
		std::shared_ptr<MaterialDefinition> materialDefinition;

		void updateUniforms();
	};
}
