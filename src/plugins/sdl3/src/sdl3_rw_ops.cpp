#include "sdl3_rw_ops.h"
#include "halley/text/string_converter.h"

using namespace Halley;

std::unique_ptr<ResourceDataReader> SDL3RWOps::fromPath(const String& path, int64_t start, int64_t end)
{
	auto fp = SDL_IOFromFile(path.c_str(), "rb");
	if (!fp) {
		return {};
	}
	return std::make_unique<SDL3RWOps>(fp, start, end, true);
}

std::unique_ptr<ResourceDataReader> SDL3RWOps::fromMemory(gsl::span<const std::byte> span)
{
	auto fp = SDL_IOFromConstMem(span.data(), int(span.size()));
	if (!fp) {
        return {};
	}
	return std::make_unique<SDL3RWOps>(fp, 0, 0, true);
}

SDL3RWOps::SDL3RWOps(SDL_IOStream* _fp, int64_t _start, int64_t _end, bool _closeOnFinish)
	: fp(_fp)
	, curPos(_start)
	, start(_start)
	, end(_end)
	, closeOnFinish(_closeOnFinish)
{
	Expects(fp);

	if (end == -1) {
		SDL_SeekIO(fp, 0, SDL_IO_SEEK_END);
		end = SDL_TellIO(fp);
		SDL_SeekIO(fp, 0, SDL_IO_SEEK_SET);
	}

	int64_t size = end - start;
	if (size < 0) {
		throw Exception("Invalid file size for resource: " + toString(size) + " bytes.", HalleyExceptions::SystemPlugin);
	}
}

SDL3RWOps::~SDL3RWOps()
{
	SDL3RWOps::close();
}

size_t SDL3RWOps::size() const
{
	return size_t(end - start);
}

int SDL3RWOps::read(gsl::span<std::byte> dst)
{
	return readAt(dst, curPos);
}

int SDL3RWOps::readAt(gsl::span<std::byte> dst, size_t pos)
{
	if (!fp) {
		return -1;
	}

	size_t toRead = std::min(dst.size(), static_cast<size_t>(end) - pos);

	UniqueLock lock(mutex);
	SDL_SeekIO(fp, pos, SDL_IO_SEEK_SET);
	const auto n = static_cast<int64_t>(SDL_ReadIO(fp, dst.data(), static_cast<int>(toRead)));
	curPos = pos + std::max(n, static_cast<int64_t>(0));

	return static_cast<int>(n);
}

void SDL3RWOps::close()
{
	if (fp) {
		if (closeOnFinish) {
			SDL_CloseIO(fp);
		}
		fp = nullptr;
		curPos = end;
	}
}

void SDL3RWOps::seek(int64_t offset, int whence)
{
	UniqueLock lock(mutex);

	if (whence == SEEK_SET) {
		curPos = offset + start;
	} else if (whence == SEEK_CUR) {
		curPos += offset;
	} else if (whence == SEEK_END) {
		curPos = end + offset;
	}
	
	SDL_SeekIO(fp, curPos, SDL_IO_SEEK_SET);
}

size_t SDL3RWOps::tell() const
{
	return size_t(curPos - start);
}
