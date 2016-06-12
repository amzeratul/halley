#include "halley/core/graphics/texture.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/texture_descriptor.h"
#include <halley/file_formats/image.h>
#include <halley/resources/metadata.h>

using namespace Halley;

std::unique_ptr<Texture> Texture::loadResource(ResourceLoader& loader)
{
	Metadata meta = loader.getMeta();
	auto video = loader.getAPI().video;

	auto future = loader.getAsync().then([meta] (std::unique_ptr<ResourceDataStatic> data) -> std::unique_ptr<Image>
	{
		return std::make_unique<Image>(data->getPath(), static_cast<Byte*>(data->getData()), data->getSize(), meta.getBool("premultiply", true));
	}).then(Executors::getMainThread(), [meta, video] (std::unique_ptr<Image> img) -> std::unique_ptr<Texture>
	{
		TextureDescriptor descriptor(Vector2i(img->getWidth(), img->getHeight()));
		descriptor.pixelData = img->getPixels();

		descriptor.useFiltering = meta.getBool("filtering", false);
		descriptor.useMipMap = meta.getBool("mipmap", false);
		descriptor.format = meta.getString("format", "RGBA") == "RGBA" ? TextureFormat::RGBA : TextureFormat::RGB;

		return video->createTexture(descriptor);
	});

	Executor e(Executors::getMainThread());
	while (!future.isReady()) {
		e.runPending();
	}

	return future.get();
}
