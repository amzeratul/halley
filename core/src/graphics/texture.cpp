#include "texture.h"
#include "../resources/resources.h"
#include "../api/halley_api.h"
#include "texture_descriptor.h"

using namespace Halley;

std::unique_ptr<Texture> Texture::loadResource(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	auto img = std::make_unique<Image>(data->getPath(), static_cast<Byte*>(data->getData()), data->getSize(), true);
	
	TextureDescriptor descriptor;
	descriptor.w = img->getWidth();
	descriptor.h = img->getHeight();
	descriptor.pixelData = img->getPixels();

	return loader.getAPI().video->createTexture(descriptor);
}
