#include "material.h"
#include "material_definition.h"
#include "material_parameter.h"
#include "../painter.h"

using namespace Halley;

static Material* currentMaterial = nullptr;
static int currentPass = 0;

Material::Material(std::shared_ptr<MaterialDefinition> materialDefinition)
	: materialDefinition(materialDefinition)
{
	for (auto& uniform : materialDefinition->getUniforms()) {
		uniforms.push_back(MaterialParameter(*this, uniform.name, uniform.type));
	}

	updateUniforms();
}

void Material::bind(int pass, Painter& painter)
{
	// Avoid redundant work
	if (currentMaterial == this && currentPass == pass && !dirty) {
		return;
	}
	currentMaterial = this;
	currentPass = pass;

	materialDefinition->bind(pass, painter);

	for (auto& u : uniforms) {
		u.bind(pass);
	}
}

void Material::updateUniforms()
{
	int tu = 0;
	for (auto& u : uniforms) {
		u.updateAddresses();
		if (u.needsTextureUnit) {
			u.textureUnit = tu++;
		} else {
			u.textureUnit = -1;
		}
	}
}

MaterialParameter& Material::operator[](String name)
{
	for (auto& u : uniforms) {
		if (u.name == name) {
			return u;
		}
	}
	throw Exception("Uniform not available: " + name);
}
