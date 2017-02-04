#include "halley/core/graphics/texture.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/texture_descriptor.h"
#include <halley/file_formats/image.h>
#include <halley/resources/metadata.h>
#include "halley/concurrency/concurrent.h"
#include <gsl/gsl>

using namespace Halley;

struct ImageData
{
public:
	ImageData(std::unique_ptr<Image> img)
		: img(std::move(img)), isRaw(false)
	{}

	ImageData(Bytes&& bytes)
		: rawBytes(std::move(bytes)), isRaw(true)
	{}

	ImageData(ImageData&& other) noexcept
		: img(std::move(other.img)), rawBytes(std::move(other.rawBytes)), isRaw(other.isRaw)
	{}

	ImageData(gsl::span<const gsl::byte> bytes)
		: rawBytes(bytes.size_bytes())
		, isRaw(true)
	{
		memcpy(rawBytes.data(), bytes.data(), bytes.size_bytes());
	}

	ImageData& operator=(ImageData&& other)
	{
		img = std::move(other.img);
		rawBytes = std::move(other.rawBytes);
		isRaw = std::move(other.isRaw);
		return *this;
	}

	Byte* getBytes()
	{
		return isRaw ? rawBytes.data() : reinterpret_cast<Byte*>(img->getPixels());
	}

private:
	std::unique_ptr<Image> img;
	Bytes rawBytes;
	bool isRaw = false;
};

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
	.then([premultiply, texture](std::unique_ptr<ResourceDataStatic> data) -> ImageData
	{
		auto& meta = texture->getMeta();
		if (meta.getString("compression") == "png") {
			return ImageData(std::make_unique<Image>(data->getPath(), data->getSpan(), premultiply));
		} else {
			return ImageData(data->getSpan());
		}
	})
	.then(Executors::getVideoAux(), [texture](ImageData img)
	{
		auto& meta = texture->getMeta();

		Vector2i size(meta.getInt("width"), meta.getInt("height"));
		TextureDescriptor descriptor(size);
		descriptor.useFiltering = meta.getBool("filtering", false);
		descriptor.useMipMap = meta.getBool("mipmap", false);
		descriptor.format = meta.getString("format", "RGBA") == "RGBA" ? TextureFormat::RGBA : TextureFormat::RGB;

		TextureDescriptor d = descriptor;
		d.pixelData = img.getBytes();
		texture->load(d);
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
