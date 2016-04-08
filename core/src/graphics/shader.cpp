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
	// TODO: parse YAML

	String vertex;
	String pixel;
	String geometry;

	String vertexFile = "shaders/sprite.vertex.glsl";
	String pixelFile = "shaders/sprite.pixel.glsl";
	String geometryFile = "";

	if (vertex == "" && vertexFile != "") {
		vertex = api.core->getResources().get<TextFile>(vertexFile)->data;
	}
	if (pixel == "" && pixelFile != "") {
		pixel = api.core->getResources().get<TextFile>(pixelFile)->data;
	}
	if (geometryFile == "" && geometryFile != "") {
		geometry = api.core->getResources().get<TextFile>(geometryFile)->data;
	}

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
