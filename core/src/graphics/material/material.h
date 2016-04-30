#pragma once

namespace Halley
{
	class Painter;
	class MaterialDefinition;
	
	class Material
	{
		friend class MaterialParameter;

	public:
		explicit Material(std::shared_ptr<MaterialDefinition> materialDefinition);
		void bind(int pass, Painter& painter);

		MaterialParameter& operator[](String name);

		MaterialDefinition& getDefinition() { return *materialDefinition; }
		const MaterialDefinition& getDefinition() const { return *materialDefinition; }

	private:
		bool dirty = false;

		std::vector<MaterialParameter> uniforms;
		std::shared_ptr<MaterialDefinition> materialDefinition;

		void updateUniforms();
	};
}
