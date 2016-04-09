#include "material_parameter.h"
#include "uniform_type.h"
#include "material.h"
#include "../api/halley_api.h"
#include "shader.h"
#include "texture.h"

using namespace Halley;


MaterialParameter::MaterialParameter(Material& material, String name)
	: material(material)
	, name(name)
{
}

VideoAPIInternal& MaterialParameter::getAPI()
{
	return *static_cast<VideoAPIInternal*>(material.api);
}

unsigned int MaterialParameter::getAddress()
{
	return material.passes[0].getShader().getUniformLocation(name);
}

void MaterialParameter::apply()
{
	toApply(*this);
}

void MaterialParameter::bind()
{
	toBind(*this);
}

void MaterialParameter::operator=(std::shared_ptr<Texture> texture)
{
	needsTextureUnit = true;
	toApply = [texture](MaterialParameter& p0) {
		p0.toBind = [texture](MaterialParameter& p) {
			texture->bind(p.textureUnit);
			p.getAPI().getUniformBinding(p.getAddress(), UniformType::Int, 1, &p.textureUnit);
		};
	};
}

void MaterialParameter::operator=(Colour colour)
{
	needsTextureUnit = false;
	toApply = [colour](MaterialParameter& p) {
		std::array<float, 4> col = { colour.r, colour.g, colour.b, colour.a };
		p.toBind = p.getAPI().getUniformBinding(p.getAddress(), UniformType::Float, 4, col.data());
	};
}

void MaterialParameter::operator=(float p)
{
	needsTextureUnit = false;
	toApply = [p](MaterialParameter& t) {
		auto v = p;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Float, 1, &v);
	};
}

void MaterialParameter::operator=(Vector2f p)
{
	needsTextureUnit = false;
	toApply = [p](MaterialParameter& t) {
		auto v = p;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Float, 2, &v);
	};
}

void MaterialParameter::operator=(int p)
{
	needsTextureUnit = false;
	toApply = [p](MaterialParameter& t) {
		auto v = p;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Int, 1, &v);
	};
}

void MaterialParameter::operator=(Vector2i p)
{
	needsTextureUnit = false;
	toApply = [p](MaterialParameter& t) {
		auto v = p;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Int, 2, &v);
	};
}

void MaterialParameter::operator=(Matrix4f m)
{
	needsTextureUnit = false;
	toApply = [m](MaterialParameter& t) {
		auto v = m;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Mat4, 1, &v);
	};
}
