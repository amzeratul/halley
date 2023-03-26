#include "halley/file_formats/hlif_file.h"

#include "halley/bytes/compression.h"

using namespace Halley;

void HLIFFile::decode(Image& dst, gsl::span<const gsl::byte> bytes)
{
	if (!isHLIF(bytes)) {
		throw Exception("Not an HLIF file.", HalleyExceptions::Utils);
	}

	Header header;
	if (bytes.size() < sizeof(header)) {
		throw Exception("Invalid HLIF file.", HalleyExceptions::Utils);
	}
	memcpy(&header, bytes.data(), sizeof(header));

	const int bpp = getBPP(header.format);
	if (header.uncompressedSize != static_cast<uint32_t>(header.width * header.height * bpp + header.height)) {
		throw Exception("Invalid HLIF file encoding.", HalleyExceptions::Utils);
	}

	Bytes decompressedData;
	decompressedData.resize(header.uncompressedSize);
	const auto decompressedSize = Compression::lz4Decompress(bytes.subspan(sizeof(header), header.compressedSize), decompressedData.byte_span());
	if (!decompressedSize) {
		throw Exception("Error decoding HLIF file.", HalleyExceptions::Utils);
	}
	decompressedData.resize(*decompressedSize);

	const auto dataSpan = gsl::span<Byte>(decompressedData);
	const auto lineData = dataSpan.subspan(0, header.height);
	const auto pixelData = dataSpan.subspan(header.height);
	const auto imgFormat = header.format == Format::RGBA ?
		((header.flags & static_cast<uint8_t>(Flags::Premultiplied)) ? Image::Format::RGBAPremultiplied : Image::Format::RGBA) :
		(header.format == Format::SingleChannel ? Image::Format::SingleChannel : Image::Format::Indexed);
	const auto imgSize = Vector2i(header.width, header.height);

	decodeLines(header.format, imgSize, lineData, pixelData);

	dst = Image(imgFormat, imgSize);
	memcpy(dst.getPixelBytes().data(), pixelData.data(), pixelData.size_bytes());
}

Bytes HLIFFile::encode(const Image& image)
{
	// Fill header
	Header header;
	memcpy(header.id, hlifId, 8);
	switch (image.getFormat()) {
	case Image::Format::RGBA:
		header.format = Format::RGBA;
		break;
	case Image::Format::RGBAPremultiplied:
		header.format = Format::RGBA;
		header.flags |= static_cast<uint8_t>(Flags::Premultiplied);
		break;
	case Image::Format::Indexed:
		header.format = Format::Indexed;
		break;
	case Image::Format::SingleChannel:
		header.format = Format::SingleChannel;
		break;
	default:
		throw Exception("Cannot convert this image format into HLIF: " + toString(image.getFormat()), HalleyExceptions::Utils);
	}
	header.width = static_cast<uint16_t>(image.getWidth());
	header.height = static_cast<uint16_t>(image.getHeight());
	const int bpp = getBPP(header.format);
	header.uncompressedSize = header.width * header.height * bpp + header.height;

	// Prepare uncompressed data
	Bytes uncompressed(header.uncompressedSize);
	const auto dataSpan = gsl::span<Byte>(uncompressed);
	const auto lineSpan = dataSpan.subspan(0, header.height);
	const auto pixelSpan = dataSpan.subspan(header.height);
	memset(lineSpan.data(), 0, lineSpan.size_bytes());
	memcpy(pixelSpan.data(), image.getPixelBytes().data(), pixelSpan.size());

	// Try compressing with no filters first
	Compression::LZ4Options options;
	options.mode = Compression::LZ4Mode::HC;
	auto compressedUnfiltered = Compression::lz4Compress(gsl::as_bytes(dataSpan), options);

	// Filter and compress again
	encodeLines(header.format, image.getSize(), lineSpan, pixelSpan);
	auto compressedFiltered = Compression::lz4Compress(gsl::as_bytes(dataSpan), options);
	uncompressed = {};

	// Take the best of the two
	const auto compressed = compressedUnfiltered.size() < compressedFiltered.size() ? std::move(compressedUnfiltered) : std::move(compressedFiltered);
	compressedFiltered = {};
	compressedUnfiltered = {};

	// Finish header and generate final bytes
	header.compressedSize = static_cast<uint32_t>(compressed.size());
	Bytes finalData(sizeof(header) + compressed.size());
	memcpy(finalData.data(), &header, sizeof(header));
	memcpy(finalData.data() + sizeof(header), compressed.data(), compressed.size());
	return finalData;
}

HLIFFile::Info HLIFFile::getInfo(gsl::span<const gsl::byte> bytes)
{
	if (!isHLIF(bytes)) {
		throw Exception("Not an HLIF file.", HalleyExceptions::Utils);
	}

	Header header;
	if (bytes.size() < sizeof(header)) {
		throw Exception("Invalid HLIF file.", HalleyExceptions::Utils);
	}

	memcpy(&header, bytes.data(), sizeof(header));
	
	const auto imgFormat = header.format == Format::RGBA ? Image::Format::RGBA : (header.format == Format::SingleChannel ? Image::Format::SingleChannel : Image::Format::Indexed);
	const auto imgSize = Vector2i(header.width, header.height);
	return Info{ imgSize, imgFormat };
}

bool HLIFFile::isHLIF(gsl::span<const gsl::byte> bytes)
{
	return bytes.size() >= 8 && memcmp(bytes.data(), hlifId, 8) == 0;
}

void HLIFFile::decodeLines(Format format, Vector2i size, gsl::span<const uint8_t> lineData, gsl::span<uint8_t> pixelData)
{
	const auto bpp = getBPP(format);
	const auto stride = bpp * size.x;

	Bytes blankLine(stride, 0);
	gsl::span<const uint8_t> prevLine = blankLine;

	for (int y = 0; y < size.y; ++y) {
		const auto curLine = pixelData.subspan(y * stride, stride);
		decodeLine(static_cast<LineEncoding>(lineData[y]), curLine, prevLine, bpp);
		prevLine = curLine;
	}
}

void HLIFFile::encodeLines(Format format, Vector2i size, gsl::span<uint8_t> lineData, gsl::span<uint8_t> pixelData)
{
	const auto bpp = getBPP(format);
	const auto stride = bpp * size.x;
	assert(lineData.size_bytes() == size.y);
	assert(pixelData.size_bytes() == stride * size.y);

	Bytes blankLine(stride, 0);
	
	for (int y = size.y - 1; --y >= 0;) {
		const auto curLine = pixelData.subspan(y * stride, stride);
		const auto prevLine = y > 0 ? pixelData.subspan((y - 1) * stride, stride) : gsl::span<const uint8_t>(blankLine);
		const auto encoding = findBestLineEncoding(curLine, prevLine, bpp);
		encodeLine(encoding, curLine, prevLine, bpp);
		lineData[y] = static_cast<uint8_t>(encoding);
	}
}

namespace {
	static int16_t getClosest(int16_t a, int16_t b, int16_t c, int16_t p)
	{
		const auto da = std::abs(a - p);
		const auto db = std::abs(b - p);
		const auto dc = std::abs(c - p);
		if (da <= db && da <= dc) {
			return a;
		} else if (db <= dc) {
			return b;
		} else {
			return c;
		}
	}

	static uint32_t getAbsDistance(uint8_t v)
	{
		if (v <= 127) {
			return v;
		} else {
			return static_cast<uint8_t>(256 - static_cast<uint32_t>(v));
		}
	}
}

HLIFFile::LineEncoding HLIFFile::findBestLineEncoding(gsl::span<const uint8_t> curLine, gsl::span<const uint8_t> prevLine, int bpp)
{
	// Try all five filters and accumulate the values
	std::array<uint32_t, 5> accumulator;
	accumulator.fill(0);
	
	const size_t n = curLine.size();
	for (size_t x = bpp; x < n; ++x) {
		const uint8_t cur = curLine[x];
		const uint8_t a = curLine[x - bpp];
		const uint8_t b = prevLine[x];
		const uint8_t c = prevLine[x - bpp];
		const int16_t p = static_cast<int16_t>(a) + static_cast<int16_t>(b) - static_cast<int16_t>(c);
		const uint8_t pc = static_cast<uint8_t>(getClosest(a, b, c, p));
		const uint8_t avg = static_cast<uint8_t>((static_cast<uint16_t>(a) + static_cast<uint16_t>(b)) / 2);

		accumulator[static_cast<uint8_t>(LineEncoding::None)] += getAbsDistance(cur);
		accumulator[static_cast<uint8_t>(LineEncoding::Sub)] += getAbsDistance(cur - a);
		accumulator[static_cast<uint8_t>(LineEncoding::Up)] += getAbsDistance(cur - b);
		accumulator[static_cast<uint8_t>(LineEncoding::Average)] += getAbsDistance(cur - avg);
		accumulator[static_cast<uint8_t>(LineEncoding::Paeth)] += getAbsDistance(cur - pc);
	}

	return static_cast<LineEncoding>(std::min_element(accumulator.begin(), accumulator.end()) - accumulator.begin());
}

void HLIFFile::encodeLine(LineEncoding lineEncoding, gsl::span<uint8_t> curLine, gsl::span<const uint8_t> prevLine, int bpp)
{
	const size_t n = curLine.size();

	switch (lineEncoding) {
	case LineEncoding::None:
		break;
	case LineEncoding::Sub:
		for (int x = static_cast<int>(n); --x >= bpp;) {
			curLine[x] -= curLine[x - bpp];
		}
		break;
	case LineEncoding::Up:
		for (size_t x = 0; x < n; ++x) {
			curLine[x] -= prevLine[x];
		}
		break;
	case LineEncoding::Average:
		for (int x = static_cast<int>(n); --x >= bpp;) {
			const uint8_t a = curLine[x - bpp];
			const uint8_t b = prevLine[x];
			const uint8_t avg = static_cast<uint8_t>((static_cast<uint16_t>(a) + static_cast<uint16_t>(b)) / 2);
			curLine[x] -= avg;
		}
		break;
	case LineEncoding::Paeth:
		for (int x = static_cast<int>(n); --x >= bpp;) {
			const uint8_t a = curLine[x - bpp];
			const uint8_t b = prevLine[x];
			const uint8_t c = prevLine[x - bpp];
			const int16_t p = static_cast<int16_t>(a) + static_cast<int16_t>(b) - static_cast<int16_t>(c);
			const uint8_t pc = static_cast<uint8_t>(getClosest(a, b, c, p));
			curLine[x] -= pc;
		}
		break;
	}
}

void HLIFFile::decodeLine(LineEncoding lineEncoding, gsl::span<uint8_t> curLine, gsl::span<const uint8_t> prevLine, int bpp)
{
	const size_t n = curLine.size();

	switch (lineEncoding) {
	case LineEncoding::None:
		break;
	case LineEncoding::Sub:
		for (size_t x = bpp; x < n; ++x) {
			curLine[x] += curLine[x - bpp];
		}
		break;
	case LineEncoding::Up:
		for (size_t x = 0; x < n; ++x) {
			curLine[x] += prevLine[x];
		}
		break;
	case LineEncoding::Average:
		for (size_t x = bpp; x < n; ++x) {
			const uint8_t a = curLine[x - bpp];
			const uint8_t b = prevLine[x];
			const uint8_t avg = static_cast<uint8_t>((static_cast<uint16_t>(a) + static_cast<uint16_t>(b)) / 2);
			curLine[x] += avg;
		}
		break;
	case LineEncoding::Paeth:
		for (size_t x = bpp; x < n; ++x) {
			const uint8_t a = curLine[x - bpp];
			const uint8_t b = prevLine[x];
			const uint8_t c = prevLine[x - bpp];
			const int16_t p = static_cast<int16_t>(a) + static_cast<int16_t>(b) - static_cast<int16_t>(c);
			const uint8_t pc = static_cast<uint8_t>(getClosest(a, b, c, p));
			curLine[x] += pc;
		}
		break;
	}
}

int HLIFFile::getBPP(Format format)
{
	return format == Format::RGBA ? 4 : 1;
}
