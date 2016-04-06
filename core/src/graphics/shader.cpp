#include "shader.h"
#include "../resources/resources.h"
#include "../api/halley_api.h"

using namespace Halley;

Shader::Shader(String name)
	: name(name)
{
}

std::unique_ptr<Shader> Shader::loadResource(ResourceLoader& loader)
{
	auto& api = loader.getAPI();

	auto data = loader.getStatic();
	auto shaderStrData = data->getString();
	// TODO: parse

	String vertex;
	String pixel;
	String geometry;

	// TODO: read these from the right place
	vertex = api.core->getResources().get<TextFile>("shaders/sprite.vertex.glsl")->data;
	pixel = api.core->getResources().get<TextFile>("shaders/sprite.pixel.glsl")->data;

	auto shader = api.video->createShader(loader.getName());
	if (vertex != "") {
		shader->addVertexSource(vertex);
	}
	if (pixel != "") {
		shader->addPixelSource(pixel);
	}
	if (geometry != "") {
		shader->addGeometrySource(geometry);
	}
	shader->compile();

	return shader;
}
