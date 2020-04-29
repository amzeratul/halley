#include "halley/core/graphics/texture_descriptor.h"

using namespace Halley;

TextureDescriptorImageData::TextureDescriptorImageData()
{}

TextureDescriptorImageData::TextureDescriptorImageData(std::unique_ptr<Image> img)
	: img(move(img))
	, isRaw(false)
{
}

TextureDescriptorImageData::TextureDescriptorImageData(Bytes&& bytes, std::optional<int> stride)
	: rawBytes(move(bytes))
	, stride(stride)
	, isRaw(true)
{}

TextureDescriptorImageData::TextureDescriptorImageData(TextureDescriptorImageData&& other) noexcept
	: img(move(other.img))
	, rawBytes(move(other.rawBytes))
	, stride(other.stride)
	, isRaw(other.isRaw)
{}

TextureDescriptorImageData::TextureDescriptorImageData(gsl::span<const gsl::byte> bytes, std::optional<int> stride)
	: rawBytes(bytes.size_bytes())
	, stride(stride)
    , isRaw(true)
{
	memcpy(rawBytes.data(), bytes.data(), bytes.size_bytes());
}

TextureDescriptorImageData& TextureDescriptorImageData::operator=(TextureDescriptorImageData&& other) noexcept
{
	img = move(other.img);
	rawBytes = move(other.rawBytes);
	stride = other.stride;
	isRaw = other.isRaw;
	return *this;
}

bool TextureDescriptorImageData::empty() const
{
	return isRaw ? rawBytes.empty() : !img;
}

Byte* TextureDescriptorImageData::getBytes()
{
	return isRaw ? rawBytes.data() : reinterpret_cast<Byte*>(img->getPixelBytes().data());
}

gsl::span<const gsl::byte> TextureDescriptorImageData::getSpan() const
{
	if (isRaw) {
		return gsl::as_bytes(gsl::span<const Byte>(rawBytes.data(), rawBytes.size()));
	} else {
		return gsl::as_bytes(img->getPixelBytes());
	}
}

Bytes TextureDescriptorImageData::moveBytes()
{
	if (isRaw) {
		return move(rawBytes);
	} else {
		if (!img || img->getByteSize() == 0) {
			return Bytes();
		}
		Bytes result(img->getByteSize());
		memcpy(result.data(), img->getPixelBytes().data(), img->getByteSize());
		img.reset();
		return result;
	}
}

std::optional<int> TextureDescriptorImageData::getStride() const
{
	return stride;
}

int TextureDescriptorImageData::getStrideOr(int assumedStride) const
{
	if (stride) {
		return stride.value();
	} else {
		return assumedStride;
	}
}

TextureDescriptor::TextureDescriptor(Vector2i size, TextureFormat format)
	: size(size)
	, format(format)
{
}

TextureDescriptor& TextureDescriptor::operator=(TextureDescriptor&& other) noexcept
{
	size = other.size;
	padding = other.padding;
	format = other.format;
	pixelFormat = other.pixelFormat;
	pixelData = std::move(other.pixelData);
	useMipMap = other.useMipMap;
	useFiltering = other.useFiltering;
	addressMode = other.addressMode;
	canBeUpdated = other.canBeUpdated;
	isRenderTarget = other.isRenderTarget;
	isDepthStencil = other.isDepthStencil;
	return *this;
}

int TextureDescriptor::getBitsPerPixel(TextureFormat format)
{
	switch (format) {
	case TextureFormat::RGBA:
		return 4;
	case TextureFormat::DEPTH:
		return 3;
	case TextureFormat::RGB:
		return 3;
	case TextureFormat::Indexed:
	case TextureFormat::Red:
		return 1;
	}
	throw Exception("Unknown image format: " + toString(format), HalleyExceptions::Graphics);
}
