#include <halley/support/exception.h>
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/graphics/painter.h"

using namespace Halley;

static Material* currentMaterial = nullptr;
static int currentPass = 0;

Material::Material(const Material& other)
	: uniforms(other.uniforms)
	, materialDefinition(other.materialDefinition)
	, mainTexture(other.mainTexture)
{	
}

Material::Material(std::shared_ptr<const MaterialDefinition> materialDefinition)
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

void Material::resetBindCache()
{
	currentMaterial = nullptr;
	currentPass = 0;
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

void Material::setMainTexture(const std::shared_ptr<const Texture>& tex)
{
	mainTexture = tex;
}

const std::shared_ptr<const Texture>& Material::getMainTexture() const
{
	return mainTexture;
}

MaterialParameter& Material::getParameter(const String& name)
{
	for (auto& u : uniforms) {
		if (u.name == name) {
			return u;
		}
	}
	throw Exception("Uniform not available: " + name);
}

std::shared_ptr<Material> Material::clone() const
{
	return std::make_shared<Material>(*this);
}

Material& Material::set(const String& name, const std::shared_ptr<const Texture>& texture)
{
	getParameter(name) = texture;
	if (name == "tex0") {
		setMainTexture(texture);
	}
	return *this;
}
