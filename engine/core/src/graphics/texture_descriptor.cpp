#include "halley/core/graphics/texture_descriptor.h"

Halley::TextureDescriptorImageData::TextureDescriptorImageData()
{}

Halley::TextureDescriptorImageData::TextureDescriptorImageData(std::unique_ptr<Image> img)
	: img(std::move(img))
	, isRaw(false)
{}

Halley::TextureDescriptorImageData::TextureDescriptorImageData(Bytes&& bytes)
	: rawBytes(std::move(bytes))
	, isRaw(true)
{}

Halley::TextureDescriptorImageData::TextureDescriptorImageData(TextureDescriptorImageData&& other) noexcept
	: img(std::move(other.img))
	, rawBytes(std::move(other.rawBytes))
	, isRaw(other.isRaw)
{}

Halley::TextureDescriptorImageData::TextureDescriptorImageData(gsl::span<const gsl::byte> bytes)
	: rawBytes(bytes.size_bytes())
    , isRaw(true)
{
	memcpy(rawBytes.data(), bytes.data(), bytes.size_bytes());
}

Halley::TextureDescriptorImageData& Halley::TextureDescriptorImageData::operator=(TextureDescriptorImageData&& other) noexcept
{
	img = std::move(other.img);
	rawBytes = std::move(other.rawBytes);
	isRaw = std::move(other.isRaw);
	return *this;
}

bool Halley::TextureDescriptorImageData::empty() const
{
	return isRaw ? rawBytes.empty() : !img;
}

Halley::Byte* Halley::TextureDescriptorImageData::getBytes()
{
	return isRaw ? rawBytes.data() : reinterpret_cast<Byte*>(img->getPixels());
}

int Halley::TextureDescriptor::getBitsPerPixel(TextureFormat format)
{
	switch (format) {
	case TextureFormat::RGBA:
		return 4;
	default:
		return 3;
	}
}
