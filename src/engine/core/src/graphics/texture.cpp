#include "halley/core/graphics/texture.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/texture_descriptor.h"
#include <halley/file_formats/image.h>
#include <halley/resources/metadata.h>
#include "halley/concurrency/concurrent.h"

using namespace Halley;

Texture::Texture(Vector2i size)
	: size(size)
{}

std::shared_ptr<Texture> Texture::loadResource(ResourceLoader& loader)
{
	auto& meta = loader.getMeta();
	Vector2i size(meta.getInt("width", -1), meta.getInt("height", -1));
	if (size.x == -1 && size.y == -1) {
		throw Exception("Unable to load texture \"" + loader.getName() + "\" due to missing asset data.");
	}

	std::shared_ptr<Texture> texture = loader.getAPI().video->createTexture(size);
	texture->setMeta(meta);

	loader.getAsync()
	.then([texture](std::unique_ptr<ResourceDataStatic> data) -> TextureDescriptorImageData
	{
		auto& meta = texture->getMeta();
		if (meta.getString("compression") == "png") {
			return TextureDescriptorImageData(std::make_unique<Image>(*data, meta));
		} else {
			return TextureDescriptorImageData(data->getSpan());
		}
	})
	.then(Executors::getVideoAux(), [texture](TextureDescriptorImageData img)
	{
		auto& meta = texture->getMeta();

		auto formatStr = meta.getString("format", "rgba");
		if (formatStr == "rgba_premultiplied") {
			formatStr = "rgba";
		}

		Vector2i size(meta.getInt("width"), meta.getInt("height"));
		TextureDescriptor descriptor(size);
		descriptor.useFiltering = meta.getBool("filtering", false);
		descriptor.useMipMap = meta.getBool("mipmap", false);
		descriptor.clamp = meta.getBool("clamp", true);
		descriptor.format = fromString<TextureFormat>(formatStr);
		descriptor.pixelData = std::move(img);
		descriptor.pixelFormat = meta.getString("compression") == "png" ? PixelDataFormat::Image : PixelDataFormat::Precompiled;
		texture->load(std::move(descriptor));
	});

	return texture;
}
