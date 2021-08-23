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
		Depth,
		Red
	};

	template <>
	struct EnumNames<TextureFormat> {
		constexpr std::array<const char*, 5> operator()() const {
			return{{
				"indexed",
				"rgb",
				"rgba",
				"depth",
				"red"
			}};
		}
	};

	enum class PixelDataFormat
	{
		Image,
		Precompiled
	};

	enum class TextureAddressMode
	{
		Repeat,
		Clamp,
		Mirror
	};

	template <>
	struct EnumNames<TextureAddressMode> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"repeat",
				"clamp",
				"mirror"
			}};
		}
	};

	
	class TextureDescriptorImageData
	{
	public:
		TextureDescriptorImageData();
		TextureDescriptorImageData(std::unique_ptr<Image> img);
		TextureDescriptorImageData(std::shared_ptr<Image> img);
		TextureDescriptorImageData(Bytes&& bytes, std::optional<int> stride = {});
		TextureDescriptorImageData(TextureDescriptorImageData&& other) noexcept;
		TextureDescriptorImageData(gsl::span<const gsl::byte> bytes, std::optional<int> stride = {});

		TextureDescriptorImageData& operator=(TextureDescriptorImageData&& other) noexcept;

		bool empty() const;
		Byte* getBytes();
		gsl::span<const gsl::byte> getSpan() const;

		Bytes moveBytes();

		std::optional<int> getStride() const;
		int getStrideOr(int assumedStride) const;

		Image* getImage();
		const Image* getImage() const;

	private:
		std::unique_ptr<Image> imgUnique;
		std::shared_ptr<Image> imgShared;
		Bytes rawBytes;
		std::optional<int> stride;
		bool isRaw = false;
	};

	class TextureDescriptor
	{
	public:
		Vector2i size;
		size_t padding = 0;
		TextureFormat format = TextureFormat::RGBA;
		PixelDataFormat pixelFormat = PixelDataFormat::Image;
		TextureAddressMode addressMode = TextureAddressMode::Clamp;
		TextureDescriptorImageData pixelData;

		bool useMipMap = false;
		bool useFiltering = false;
		bool canBeUpdated = false;
		bool isRenderTarget = false;
		bool isDepthStencil = false;
		bool isHardwareVideoDecodeTexture = false;
		bool retainPixelData = false;
		bool canBeReadOnCPU = false;

		TextureDescriptor() = default;
		TextureDescriptor(TextureDescriptor&& other) noexcept = default;
		TextureDescriptor(Vector2i size, TextureFormat format = TextureFormat::RGBA);

		TextureDescriptor& operator=(TextureDescriptor&& other) noexcept;

		static int getBitsPerPixel(TextureFormat format);
	};
}
