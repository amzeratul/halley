#include "material.h"
#include "../resources/resources.h"

using namespace Halley;

Material::Material(std::shared_ptr<Shader> shader)
{
	// TODO
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
