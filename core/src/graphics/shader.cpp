#include "shader.h"
#include "../resources/resources.h"
#include "../api/halley_api.h"

using namespace Halley;

void Shader::ensureLoaded()
{
	// TODO
}

std::unique_ptr<Shader> Shader::loadResource(ResourceLoader loader)
{
	auto shader = loader.getAPI().video->createShader();
	// TODO: do stuff
	return shader;
}
