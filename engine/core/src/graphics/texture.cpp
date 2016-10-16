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
	Vector2i size(meta.getInt("width", -1), meta.getInt("height", -1));
	if (size.x == -1 && size.y == -1) {
		throw Exception("Unable to load texture \"" + loader.getName() + "\" due to missing asset data.");
	}
	bool premultiply = meta.getBool("premultiply", true);

	std::shared_ptr<Texture> texture = loader.getAPI().video->createTexture(size);
	texture->setMeta(meta);
	texture->computeSlice();

	loader.getAsync()
	.then([premultiply](std::unique_ptr<ResourceDataStatic> data) -> std::unique_ptr<Image>
	{
		return std::make_unique<Image>(data->getPath(), data->getSpan(), premultiply);
	})
	.then(Executors::getVideoAux(), [texture](std::unique_ptr<Image> img)
	{
		auto& meta = texture->getMeta();

		Vector2i size(meta.getInt("width"), meta.getInt("height"));
		TextureDescriptor descriptor(size);
		descriptor.useFiltering = meta.getBool("filtering", false);
		descriptor.useMipMap = meta.getBool("mipmap", false);
		descriptor.format = meta.getString("format", "RGBA") == "RGBA" ? TextureFormat::RGBA : TextureFormat::RGB;

		if (img->getSize() != descriptor.size) {
			throw Exception("Image size does not match metadata.");
		}
		TextureDescriptor d = descriptor;
		d.pixelData = img->getPixels();
		texture->load(d);
	});

	return texture;
}

Vector4i Texture::getSlice() const
{
	return slice;
}

void Texture::computeSlice()
{
	auto& meta = getMeta();
	slice.x = meta.getInt("slice_left", 0);
	slice.y = meta.getInt("slice_top", 0);
	slice.z = meta.getInt("slice_right", 0);
	slice.w = meta.getInt("slice_bottom", 0);
}
