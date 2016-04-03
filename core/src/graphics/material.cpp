#include "material.h"
#include "../resources/resources.h"
#include "shader.h"

using namespace Halley;

Material::Material(std::shared_ptr<Shader> _shader)
	: shader(_shader)
{
}

void Material::bind()
{
	if (!shader) {
		throw new Exception("Material has no shader.");
	}

	ensureLoaded();

	shader->bind();
	shader->bindUniforms(uniforms);
}

void Material::ensureLoaded()
{
	// TODO?
}

MaterialParameter Material::operator[](String name)
{
	return MaterialParameter(*this, name);
}

std::unique_ptr<Material> Material::loadResource(ResourceLoader loader)
{
	// TODO
	return std::make_unique<Material>(std::shared_ptr<Shader>());
}

MaterialParameter::MaterialParameter(Material& material, String name)
	: material(material)
	, name(name)
{
}

void MaterialParameter::operator=(std::shared_ptr<Texture> texture)
{
	auto location = material.shader->getUniformLocation(name);
	// TODO
}
