#include "shader.h"
#include "../resources/resources.h"
#include "../api/halley_api.h"

using namespace Halley;

std::unique_ptr<Shader> Shader::loadResource(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	auto shaderStrData = data->getString();
	// TODO: parse string data

	auto shader = loader.getAPI().video->createShader();

	// TODO: read sources
	//shader->addVertexSource("");
	//shader->addPixelSource("");

	return shader;
}
