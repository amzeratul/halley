#include "halley/graphics/texture.h"
#include "halley/api/halley_api.h"
#include "halley/graphics/texture_descriptor.h"
#include <halley/file_formats/image.h>
#include <halley/resources/metadata.h>
#include "halley/concurrency/concurrent.h"
#include "halley/game/game_platform.h"
#include "halley/support/logger.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/support/debug.h"

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
	startLoading();
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
		throw Exception("Incompatible image (" + toString(image.getSize().x) + "x" + image.getSize().y + ") and texture (" + getSize().x + "x" + getSize().y + ") sizes.", HalleyExceptions::Graphics);
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

ResourceMemoryUsage Texture::getEstimatedMemoryUsage() const
{
	const auto format = expectedTextureFormat.value_or(TextureFormat::RGBA);

	const auto estimatedMaskSize = size.x * size.y / 8;
	const auto estimatedVRAM = size.x * size.y * TextureDescriptor::getBytesPerPixel(format);

	ResourceMemoryUsage result;
	result.ramUsage = sizeof(*this) + estimatedMaskSize;
	result.vramUsage = estimatedVRAM;
	return result;
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
	retainPixelData = other.retainPixelData;
	unloadable = other.unloadable;
	loaderFunc = std::move(other.loaderFunc);
	expectedTextureFormat = other.expectedTextureFormat;
}

bool Texture::canUnload() const
{
	return unloadable;
}

bool Texture::doRequestLoading()
{
	loadFromDisk();
	return true;
}

bool Texture::doRequestUnloading()
{
	clearTexture();
	descriptor = {};
	mask = {};
	doneUnloading();
	return true;
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
	texture->retainPixelData = loader.getResources().getOptions().retainPixelData;
	texture->loaderFunc = loader.getLoaderFunction(true);
	texture->unloadable = true;
	texture->expectedTextureFormat = getTextureFormat(meta);
	//texture->loadFromDisk();

	return texture;
}

void Texture::loadFromDisk()
{
	if (loaderFunc) {
		loadFromDisk(loaderFunc, retainPixelData);
	}
}

void Texture::loadFromDisk(const ResourceLoader::LoaderFunc& loaderFunc, bool retainPixelData)
{
	startLoading();
	auto texture = shared_from_this();

	Concurrent::execute(Executors::getDiskIO(), loaderFunc)
		.then([texture] (std::unique_ptr<ResourceDataStatic> data) -> std::pair<TextureDescriptorImageData, ImageMask>
	{
		const auto& meta = texture->getMeta();
		const auto& compression = meta.getString("compression");
		const bool masked = meta.getBool("withMask", false);

		try {
			Bytes imageBytes;
			ImageMask alphaMask;
			if (masked) {
				const auto options = SerializerOptions(SerializerOptions::maxVersion);
				auto imageDataAndMask = Deserializer::fromBytes<ImageDataAndMask>(data->getSpan(), options);
				imageBytes = std::move(imageDataAndMask.imageData);
				alphaMask = std::move(imageDataAndMask.mask);
			}

			if (compression == "png" || compression == "qoi" || compression == "hlif") {
				gsl::span<const std::byte> imageData = imageBytes.empty() ? data->getSpan() : imageBytes.byte_span();

				const auto format = fromString<Image::Format>(meta.getString("format", "undefined"));
				auto image = std::make_unique<Image>(imageData, format);
				HalleyAssertDev(image->getSize() == texture->getSize());
				alphaMask = ImageMask::fromAlpha(*image);
				return { TextureDescriptorImageData(std::move(image)), std::move(alphaMask) };
			} else {
				if (imageBytes.empty()) {
					return { TextureDescriptorImageData(data->getSpan()), std::move(alphaMask) };
				} else {
					return { TextureDescriptorImageData(std::move(imageBytes)), std::move(alphaMask) };
				}
			}
		} catch (...) {
			Logger::logError("Exception when trying to load texture " + texture->getAssetId() + " (compression = \"" + compression + "\", masked = " + masked + "\", size = " + String::prettySize(data->getSpan().size_bytes()) + ")");
			throw;
		}
	})
		.then(Executors::getVideoAux(), [texture, retainPixelData] (std::pair<TextureDescriptorImageData, ImageMask> imgPair)
	{
		auto& img = imgPair.first;
		auto& alphaMask = imgPair.second;
		auto& meta = texture->getMeta();

		const auto& compression = meta.getString("compression");
		const auto size = Vector2i(meta.getInt("width"), meta.getInt("height"));
		
		TextureDescriptor descriptor(size);
		descriptor.useFiltering = meta.getBool("filtering", false);
		descriptor.useMipMap = meta.getBool("mipmap", false);
		descriptor.addressMode = fromString<TextureAddressMode>(meta.getString("addressMode", "clamp"));
		descriptor.format = getTextureFormat(meta);
		descriptor.pixelData = std::move(img);
		descriptor.pixelFormat = compression == "png" || compression == "qoi" || compression == "hlif" ? PixelDataFormat::Image : PixelDataFormat::Precompiled;
		descriptor.retainPixelData = retainPixelData;
		texture->load(std::move(descriptor));
		texture->setAlphaMask(std::move(alphaMask));
	});
}

TextureFormat Texture::getTextureFormat(Image::Format imgFormat)
{
	switch (imgFormat) {
	case Image::Format::Indexed:
		return TextureFormat::Indexed;

	case Image::Format::RGB:
		return TextureFormat::RGB;

	case Image::Format::RGBA:
	case Image::Format::RGBAPremultiplied:
		return TextureFormat::RGBA;

	case Image::Format::SingleChannel:
		return TextureFormat::Red;

	case Image::Format::Undefined: // Hmm
	default:
		return TextureFormat::RGBA;
	}
}

TextureFormat Texture::getTextureFormat(const Metadata& meta)
{
	return getTextureFormat(fromString<Image::Format>(meta.getString("format", "rgba")));
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
