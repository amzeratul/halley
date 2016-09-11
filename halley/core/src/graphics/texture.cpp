#include "halley/core/graphics/texture.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/texture_descriptor.h"
#include <halley/file_formats/image.h>
#include <halley/resources/metadata.h>
#include "halley/concurrency/concurrent.h"

using namespace Halley;

std::shared_ptr<Texture> Texture::loadResource(ResourceLoader& loader)
{
	Metadata meta = loader.getMeta();
	auto video = loader.getAPI().video;

	auto loadImage = [meta](std::unique_ptr<ResourceDataStatic> data) -> std::unique_ptr<Image>
	{
		return std::make_unique<Image>(data->getPath(), data->getSpan(), meta.getBool("premultiply", true));
	};

	std::shared_ptr<Image> img = loader.getAsync().then(loadImage).get(); // LOL
	Vector2i size(img->getWidth(), img->getHeight());
	std::shared_ptr<Texture> texture = video->createTexture(size);

	TextureDescriptor descriptor(size);
	descriptor.useFiltering = meta.getBool("filtering", false);
	descriptor.useMipMap = meta.getBool("mipmap", false);
	descriptor.format = meta.getString("format", "RGBA") == "RGBA" ? TextureFormat::RGBA : TextureFormat::RGB;

	auto loadTexture = [img, descriptor, texture]()
	{
		if (img->getSize() != descriptor.size) {
			throw Exception("Image size does not match metadata.");
		}
		TextureDescriptor d = descriptor;
		d.pixelData = img->getPixels();
		texture->load(d);
	};

	Concurrent::execute(Executors::getVideoAux(), loadTexture);

	return texture;

	//return loader.getAsync().then(loadImage).then(Executors::getVideoAux(), loadTexture).get(); // lol innefficiency
	//return loadTexture(loadImage(loader.getStatic()));
}
