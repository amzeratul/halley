#include "win32_data_reader.h"
#include "halley/text/string_converter.h"
using namespace Halley;

Win32DataReader::Win32DataReader(String path, int64_t _start, int64_t _end)
	: start(_start)
	, end(_end)
{
#ifdef _MSC_VER
	auto result = _wfopen_s(&fp, path.getUTF16().c_str(), L"rb");
	if (result || !fp) {
		throw Exception("Unable to open file: " + path, HalleyExceptions::SystemPlugin);
	}
#else
	fp = fopen(path.c_str(), "rb");
#endif

	if (end == -1) {
		fseek(fp, 0, SEEK_END);
		end = ftell(fp);
		fseek(fp, 0, SEEK_SET);
	}

	const int64_t size = end - start;
	if (size < 0) {
		throw Exception("Invalid file size for resource: " + toString(size) + " bytes.", HalleyExceptions::SystemPlugin);
	}
}

size_t Win32DataReader::size() const
{
	return end - start;
}

int Win32DataReader::read(gsl::span<gsl::byte> dst)
{
	const size_t toRead = std::min(size_t(dst.size()), size_t(end - pos));
	fseek(fp, long(pos), SEEK_SET);
	const int n = static_cast<int>(fread(dst.data(), 1, static_cast<int>(toRead), fp));
	pos += n;
	return n;
}

void Win32DataReader::seek(int64_t offset, int whence)
{
	if (whence == SEEK_SET) {
		pos = int(offset + start);
	} else if (whence == SEEK_CUR) {
		pos += int(offset);
	} else if (whence == SEEK_END) {
		pos = int(end + offset);
	}
	fseek(fp, long(pos), SEEK_SET);
}

size_t Win32DataReader::tell() const
{
	return size_t(pos - start);
}

void Win32DataReader::close()
{
	if (fp) {
		fclose(fp);
		fp = nullptr;
		pos = end;
	}
}
