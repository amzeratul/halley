#include "halley/core/graphics/texture_descriptor.h"

using namespace Halley;

TextureDescriptorImageData::TextureDescriptorImageData()
{}

TextureDescriptorImageData::TextureDescriptorImageData(std::unique_ptr<Image> img)
	: img(move(img))
	, isRaw(false)
{}

TextureDescriptorImageData::TextureDescriptorImageData(Bytes&& bytes)
	: rawBytes(move(bytes))
	, isRaw(true)
{}

TextureDescriptorImageData::TextureDescriptorImageData(TextureDescriptorImageData&& other) noexcept
	: img(move(other.img))
	, rawBytes(move(other.rawBytes))
	, isRaw(other.isRaw)
{}

TextureDescriptorImageData::TextureDescriptorImageData(gsl::span<const gsl::byte> bytes)
	: rawBytes(bytes.size_bytes())
    , isRaw(true)
{
	memcpy(rawBytes.data(), bytes.data(), bytes.size_bytes());
}

TextureDescriptorImageData& TextureDescriptorImageData::operator=(TextureDescriptorImageData&& other) noexcept
{
	img = move(other.img);
	rawBytes = move(other.rawBytes);
	isRaw = std::move(other.isRaw);
	return *this;
}

bool TextureDescriptorImageData::empty() const
{
	return isRaw ? rawBytes.empty() : !img;
}

Byte* TextureDescriptorImageData::getBytes()
{
	return isRaw ? rawBytes.data() : reinterpret_cast<Byte*>(img->getPixels());
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
		memcpy(result.data(), img->getPixels(), img->getByteSize());
		img.reset();
		return result;
	}
}

int TextureDescriptor::getBitsPerPixel(TextureFormat format)
{
	switch (format) {
	case TextureFormat::RGBA:
		return 4;
	default:
		return 3;
	}
}
