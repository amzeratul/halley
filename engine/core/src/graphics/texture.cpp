#include "halley/core/graphics/texture.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/texture_descriptor.h"
#include <halley/file_formats/image.h>
#include <halley/resources/metadata.h>
#include "halley/concurrency/concurrent.h"
#include <gsl/gsl>

using namespace Halley;

std::shared_ptr<Texture> Texture::loadResource(ResourceLoader& loader)
{
	auto& meta = loader.getMeta();
	Vector2i size(meta.getInt("width", -1), meta.getInt("height", -1));
	if (size.x == -1 && size.y == -1) {
		throw Exception("Unable to load texture \"" + loader.getName() + "\" due to missing asset data.");
	}
	bool premultiply = meta.getBool("premultiply", true);

	std::shared_ptr<Texture> texture = loader.getAPI().video->createTexture(size);
	texture->setMeta(meta);
	texture->computeSlice();

	loader.getAsync()
	.then([premultiply, texture](std::unique_ptr<ResourceDataStatic> data) -> TextureDescriptorImageData
	{
		auto& meta = texture->getMeta();
		if (meta.getString("compression") == "png") {
			return TextureDescriptorImageData(std::make_unique<Image>(data->getPath(), data->getSpan(), premultiply));
		} else {
			return TextureDescriptorImageData(data->getSpan());
		}
	})
	.then(Executors::getVideoAux(), [texture](TextureDescriptorImageData img)
	{
		auto& meta = texture->getMeta();

		Vector2i size(meta.getInt("width"), meta.getInt("height"));
		TextureDescriptor descriptor(size);
		descriptor.useFiltering = meta.getBool("filtering", false);
		descriptor.useMipMap = meta.getBool("mipmap", false);
		descriptor.format = meta.getString("format", "RGBA") == "RGBA" ? TextureFormat::RGBA : TextureFormat::RGB;
		descriptor.pixelData = std::move(img);
		texture->load(std::move(descriptor));
	});

	return texture;
}

Vector4s Texture::getSlices() const
{
	return slices;
}

void Texture::computeSlice()
{
	auto& meta = getMeta();
	slices.x = gsl::narrow<short, int>(meta.getInt("slice_left", 0));
	slices.y = gsl::narrow<short, int>(meta.getInt("slice_top", 0));
	slices.z = gsl::narrow<short, int>(meta.getInt("slice_right", 0));
	slices.w = gsl::narrow<short, int>(meta.getInt("slice_bottom", 0));
}
