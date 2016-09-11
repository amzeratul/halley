#include "halley/core/graphics/texture.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/texture_descriptor.h"
#include <halley/file_formats/image.h>
#include <halley/resources/metadata.h>
#include "halley/concurrency/concurrent.h"

using namespace Halley;

std::shared_ptr<Texture> Texture::loadResource(ResourceLoader& loader)
{
	Metadata& meta = loader.getMeta();
	Vector2i size(meta.getInt("width"), meta.getInt("height"));
	TextureDescriptor descriptor(size);
	descriptor.useFiltering = meta.getBool("filtering", false);
	descriptor.useMipMap = meta.getBool("mipmap", false);
	descriptor.format = meta.getString("format", "RGBA") == "RGBA" ? TextureFormat::RGBA : TextureFormat::RGB;
	bool premultiply = meta.getBool("premultiply", true);

	std::shared_ptr<Texture> texture = loader.getAPI().video->createTexture(size);

	loader.getAsync()
	.then([premultiply](std::unique_ptr<ResourceDataStatic> data) -> std::unique_ptr<Image>
	{
		return std::make_unique<Image>(data->getPath(), data->getSpan(), premultiply);
	})
	.then(Executors::getVideoAux(), [descriptor, texture](std::unique_ptr<Image> img)
	{
		if (img->getSize() != descriptor.size) {
			throw Exception("Image size does not match metadata.");
		}
		TextureDescriptor d = descriptor;
		d.pixelData = img->getPixels();
		texture->load(d);
	});

	return texture;
}
