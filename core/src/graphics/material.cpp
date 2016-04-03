#include "material.h"
#include "../resources/resources.h"
#include "shader.h"

using namespace Halley;

Material::Material(std::shared_ptr<Shader> _shader)
	: shader(_shader)
{
}

void Material::ensureLoaded()
{
	if (shader) {
		shader->ensureLoaded();
	}
}

void Material::setTexture(String name, std::shared_ptr<Texture> texture)
{
	// TODO
}

std::unique_ptr<Material> Material::loadResource(ResourceLoader loader)
{
	// TODO
	return std::make_unique<Material>(std::shared_ptr<Shader>());
}
