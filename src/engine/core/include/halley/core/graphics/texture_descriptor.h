#pragma once

#include <halley/maths/vector2.h>
#include "halley/file_formats/image.h"
#include "halley/data_structures/maybe.h"

namespace Halley
{
	enum class TextureFormat
	{
		Indexed,
		RGB,
		RGBA,
		DEPTH
	};

	template <>
	struct EnumNames<TextureFormat> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"indexed",
				"rgb",
				"rgba",
				"depth"
			}};
		}
	};

	enum class PixelDataFormat
	{
		Image,
		Precompiled
	};

	
	class TextureDescriptorImageData
	{
	public:
		TextureDescriptorImageData();
		TextureDescriptorImageData(std::unique_ptr<Image> img);
		TextureDescriptorImageData(Bytes&& bytes, Maybe<int> stride = {});
		TextureDescriptorImageData(TextureDescriptorImageData&& other) noexcept;
		TextureDescriptorImageData(gsl::span<const gsl::byte> bytes, Maybe<int> stride = {});

		TextureDescriptorImageData& operator=(TextureDescriptorImageData&& other) noexcept;

		bool empty() const;
		Byte* getBytes();
		gsl::span<const gsl::byte> getSpan() const;

		Bytes moveBytes();

		Maybe<int> getStride() const;
		int getStrideOr(int assumedStride) const;

	private:
		std::unique_ptr<Image> img;
		Bytes rawBytes;
		Maybe<int> stride;
		bool isRaw = false;
	};

	class TextureDescriptor
	{
	public:
		Vector2i size;
		size_t padding = 0;
		TextureFormat format = TextureFormat::RGBA;
		PixelDataFormat pixelFormat = PixelDataFormat::Image;
		TextureDescriptorImageData pixelData;

		bool useMipMap = false;
		bool useFiltering = false;
		bool clamp = true;
		bool canBeUpdated = false;
		bool isRenderTarget = false;
		bool isDepthStencil = false;

		TextureDescriptor() = default;
		TextureDescriptor(TextureDescriptor&& other) noexcept = default;
		TextureDescriptor(Vector2i size, TextureFormat format = TextureFormat::RGBA);

		TextureDescriptor& operator=(TextureDescriptor&& other) noexcept;

		static int getBitsPerPixel(TextureFormat format);
	};
}
