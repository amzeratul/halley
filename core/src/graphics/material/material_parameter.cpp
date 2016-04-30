#include "material_parameter.h"
#include "uniform_type.h"
#include "material.h"
#include "material_definition.h"
#include "../../api/halley_api.h"
#include "../shader.h"
#include "../texture.h"

using namespace Halley;

MaterialParameter::MaterialParameter(Material& material, String name, ShaderParameterType type)
	: material(material)
	, name(name)
	, type(type)
{
	needsTextureUnit = type == ShaderParameterType::Texture2D;
}

VideoAPIInternal& MaterialParameter::getAPI()
{
	return *static_cast<VideoAPIInternal*>(material.getDefinition().api);
}

void MaterialParameter::updateAddresses()
{
	addresses.resize(material.getDefinition().passes.size());
	for (size_t i = 0; i < addresses.size(); i++) {
		addresses[i] = material.getDefinition().passes[i].getShader().getUniformLocation(name);
	}
}

unsigned int MaterialParameter::getAddress(int pass)
{
	return addresses[pass];
}

void MaterialParameter::bind(int pass)
{
	if (toBind) {
		toBind(pass);
	}
}

void MaterialParameter::operator=(std::shared_ptr<Texture> texture)
{
	assert(type == ShaderParameterType::Texture2D);
	bindFunc = getAPI().getUniformBinding(UniformType::Int, 1);
	toBind = [texture, this](int pass) {
		texture->bind(textureUnit);
		bindFunc(getAddress(pass), &textureUnit);
	};
}

void MaterialParameter::operator=(Colour colour)
{
	assert(type == ShaderParameterType::Float4);
	bindFunc = getAPI().getUniformBinding(UniformType::Float, 4);
	toBind = [=](int pass) {
		Colour c = colour;
		bindFunc(getAddress(pass), &c);
	};
}

void MaterialParameter::operator=(float p)
{
	assert(type == ShaderParameterType::Float);
	bindFunc = getAPI().getUniformBinding(UniformType::Float, 1);
	toBind = [=](int pass) {
		auto l = p;
		bindFunc(getAddress(pass), &l);
	};
}

void MaterialParameter::operator=(Vector2f p)
{
	assert(type == ShaderParameterType::Float2);
	bindFunc = getAPI().getUniformBinding(UniformType::Float, 2);
	toBind = [=](int pass) {
		auto l = p;
		bindFunc(getAddress(pass), &l);
	};
}

void MaterialParameter::operator=(int p)
{
	assert(type == ShaderParameterType::Int);
	bindFunc = getAPI().getUniformBinding(UniformType::Int, 1);
	toBind = [=](int pass) {
		auto l = p;
		bindFunc(getAddress(pass), &l);
	};
}

void MaterialParameter::operator=(Vector2i p)
{
	assert(type == ShaderParameterType::Int2);
	bindFunc = getAPI().getUniformBinding(UniformType::Int, 2);
	toBind = [=](int pass) {
		auto l = p;
		bindFunc(getAddress(pass), &l);
	};
}

void MaterialParameter::operator=(Matrix4f m)
{
	assert(type == ShaderParameterType::Matrix4);
	bindFunc = getAPI().getUniformBinding(UniformType::Mat4, 1);
	toBind = [=](int pass) {
		auto l = m;
		bindFunc(getAddress(pass), l.getElements());
	};
}
