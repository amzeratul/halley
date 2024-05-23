#include "halley/graphics/texture.h"
#include "halley/api/halley_api.h"
#include "halley/graphics/texture_descriptor.h"
#include <halley/file_formats/image.h>
#include <halley/resources/metadata.h>
#include "halley/concurrency/concurrent.h"
#include "halley/game/game_platform.h"
#include "halley/support/logger.h"
#include "halley/bytes/byte_serializer.h"

using namespace Halley;

void ImageDataAndMask::serialize(Serializer& s) const
{
	s << imageData;
	s << mask;
}

void ImageDataAndMask::deserialize(Deserializer& s)
{
	s >> imageData;
	s >> mask;
}

Texture::Texture(Vector2i size)
	: size(size)
{}

void Texture::load(TextureDescriptor desc)
{
	descriptor = std::move(desc);
	size = descriptor.size;
	doLoad(descriptor);

	if (!descriptor.retainPixelData) {
		descriptor.pixelData = TextureDescriptorImageData();
	}
}

void Texture::copyToTexture(Painter& painter, Texture& other) const
{
	if (getSize() != other.getSize()) {
		throw Exception("Size of source and destination texture don't match.", HalleyExceptions::Graphics);
	}
	doCopyToTexture(painter, other);
}

void Texture::copyToImage(Painter& painter, Image& image) const
{
	if (image.getSize() != getSize()) {
		throw Exception("Incompatible image and texture sizes.", HalleyExceptions::Graphics);
	}
	doCopyToImage(painter, image);
}

std::unique_ptr<Image> Texture::makeImage(Painter& painter) const
{
	auto image = std::make_unique<Image>(Image::Format::RGBA, getSize());
	doCopyToImage(painter, *image);
	return image;
}

const Image* Texture::tryGetOriginalImage() const
{
	if (descriptor.retainPixelData) {
		return descriptor.pixelData.getImage();
	} else {
		return nullptr;
	}
}

void Texture::generateMipMaps()
{
}

ResourceMemoryUsage Texture::getMemoryUsage() const
{
	ResourceMemoryUsage result;
	result.ramUsage = descriptor.getMemoryUsage() + sizeof(*this) + mask.getSizeBytes();
	result.vramUsage = getVRamUsage();
	return result;
}

void Texture::doLoad(TextureDescriptor& descriptor)
{
}

void Texture::doCopyToTexture(Painter& painter, Texture& other) const
{
	Logger::logWarning("Copying to texture not implemented.");
}

void Texture::doCopyToImage(Painter& painter, Image& image) const
{
	Logger::logWarning("Copying to image not implemented.");
	image.clear(Image::convertRGBAToInt(255, 0, 255));
}

size_t Texture::getVRamUsage() const
{
	return 0;
}

void Texture::moveFrom(Texture& other)
{
	size = other.size;
	descriptor = std::move(other.descriptor);
	mask = std::move(other.mask);
}

std::shared_ptr<Texture> Texture::loadResource(ResourceLoader& loader)
{
	const auto& meta = loader.getMeta();

	Vector2i size(meta.getInt("width", -1), meta.getInt("height", -1));
	if (size.x == -1 && size.y == -1) {
		return {};
	}

	std::shared_ptr<Texture> texture = loader.getAPI().video->createTexture(size);
	texture->setAssetId(loader.getName());
	texture->setMeta(meta);
	bool retain = loader.getResources().getOptions().retainPixelData;

	loader.getAsync(true)
	.then([texture](std::unique_ptr<ResourceDataStatic> data) -> std::pair<TextureDescriptorImageData, ImageMask>
	{
		auto& meta = texture->getMeta();
		const auto& compression = meta.getString("compression");

		Bytes imageBytes;
		ImageMask alphaMask;
		if (meta.getBool("withMask", false)) {
			const auto options = SerializerOptions(SerializerOptions::maxVersion);
			auto imageDataAndMask = Deserializer::fromBytes<ImageDataAndMask>(data->getSpan(), options);
			imageBytes = std::move(imageDataAndMask.imageData);
			alphaMask = std::move(imageDataAndMask.mask);
		}

		gsl::span<const gsl::byte> imageData = imageBytes.empty() ? data->getSpan() : imageBytes.byte_span();

		if (compression == "png" || compression == "qoi" || compression == "hlif") {
			const auto format = fromString<Image::Format>(meta.getString("format", "undefined"));
			auto image = std::make_unique<Image>(imageData, format);
			alphaMask = ImageMask::fromAlpha(*image);
			return { TextureDescriptorImageData(std::move(image)), std::move(alphaMask) };
		} else {
			return { TextureDescriptorImageData(imageData), std::move(alphaMask) };
		}
	})
	.then(Executors::getVideoAux(), [texture, retain](std::pair<TextureDescriptorImageData, ImageMask> imgPair)
	{
		auto& img = imgPair.first;
		auto& alphaMask = imgPair.second;
		auto& meta = texture->getMeta();

		const auto imgFormat = fromString<Image::Format>(meta.getString("format", "rgba"));
		TextureFormat format = TextureFormat::RGBA;
		switch (imgFormat) {
		case Image::Format::Indexed:
			format = TextureFormat::Indexed;
			break;
		case Image::Format::RGB:
			format = TextureFormat::RGB;
			break;
		case Image::Format::RGBA:
		case Image::Format::RGBAPremultiplied:
			format = TextureFormat::RGBA;
			break;
		case Image::Format::SingleChannel:
			format = TextureFormat::Red;
			break;
		case Image::Format::Undefined:
			format = TextureFormat::RGBA; // Hmm
		}

		const auto& compression = meta.getString("compression");
		Vector2i size(meta.getInt("width"), meta.getInt("height"));
		
		TextureDescriptor descriptor(size);
		descriptor.useFiltering = meta.getBool("filtering", false);
		descriptor.useMipMap = meta.getBool("mipmap", false);
		descriptor.addressMode = fromString<TextureAddressMode>(meta.getString("addressMode", "clamp"));
		descriptor.format = format;
		descriptor.pixelData = std::move(img);
		descriptor.pixelFormat = compression == "png" || compression == "qoi" || compression == "hlif" ? PixelDataFormat::Image : PixelDataFormat::Precompiled;
		descriptor.retainPixelData = retain;
		texture->load(std::move(descriptor));
		texture->setAlphaMask(std::move(alphaMask));
	});

	return texture;
}

void Texture::setAlphaMask(ImageMask mask)
{
	this->mask = std::move(mask);
}

bool Texture::hasOpaquePixels(Rect4i pixelBounds) const
{
	if (mask.getSize() != Vector2i()) {
		const auto rect = pixelBounds.grow(0, 0, 1, 1).intersection(mask.getRect());

		for (const auto pos: rect) {
			if (mask.isSet(pos)) {
				return true;
			}
		}
	}

	return false;
}
