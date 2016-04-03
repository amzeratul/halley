#include "texture.h"
#include "../resources/resources.h"
#include "../api/halley_api.h"
#include "texture_descriptor.h"

using namespace Halley;

std::unique_ptr<Texture> Texture::loadResource(ResourceLoader loader)
{
	// TODO: descriptor
	TextureDescriptor descriptor;
	auto tex = loader.getAPI().video->createTexture(descriptor);
	// TODO: do stuff
	return tex;
}
