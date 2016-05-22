#include "halley/core/graphics/texture.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/texture_descriptor.h"

using namespace Halley;

std::unique_ptr<Texture> Texture::loadResource(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	auto& meta = loader.getMeta();
	auto img = std::make_unique<Image>(data->getPath(), static_cast<Byte*>(data->getData()), data->getSize(), meta.getBool("premultiply", true));
	
	TextureDescriptor descriptor(Vector2i(img->getWidth(), img->getHeight()));
	descriptor.pixelData = img->getPixels();
	
	descriptor.useFiltering = meta.getBool("filtering", false);
	descriptor.useMipMap = meta.getBool("mipmap", false);
	descriptor.format = meta.getString("format", "RGBA") == "RGBA" ? TextureFormat::RGBA : TextureFormat::RGB;

	return loader.getAPI().video->createTexture(descriptor);
}
