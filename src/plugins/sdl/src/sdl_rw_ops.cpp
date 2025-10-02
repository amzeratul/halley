#include "sdl_rw_ops.h"
#include "halley/text/string_converter.h"

using namespace Halley;

std::unique_ptr<ResourceDataReader> SDLRWOps::fromPath(const String& path, int64_t start, int64_t end)
{
	auto fp = SDL_RWFromFile(path.c_str(), "rb");
	if (!fp) {
		return std::unique_ptr<ResourceDataReader>();
	}
	return std::make_unique<SDLRWOps>(fp, start, end, true);
}

std::unique_ptr<ResourceDataReader> SDLRWOps::fromMemory(gsl::span<const std::byte> span)
{
	auto fp = SDL_RWFromConstMem(span.data(), int(span.size()));
	if (!fp) {
		return std::unique_ptr<ResourceDataReader>();
	}
	return std::make_unique<SDLRWOps>(fp, 0, 0, true);
}

SDLRWOps::SDLRWOps(SDL_RWops* _fp, int64_t _start, int64_t _end, bool _closeOnFinish)
	: fp(_fp)
	, curPos(_start)
	, start(_start)
	, end(_end)
	, closeOnFinish(_closeOnFinish)
{
	Expects(fp);

	if (end == -1) {
		SDL_RWseek(fp, 0, SEEK_END);
		end = SDL_RWtell(fp);
		SDL_RWseek(fp, 0, SEEK_SET);
	}

	int64_t size = end - start;
	if (size < 0) {
		throw Exception("Invalid file size for resource: " + toString(size) + " bytes.", HalleyExceptions::SystemPlugin);
	}
}

SDLRWOps::~SDLRWOps()
{
	SDLRWOps::close();
}

size_t SDLRWOps::size() const
{
	return size_t(end - start);
}

int SDLRWOps::read(gsl::span<std::byte> dst)
{
	return readAt(dst, curPos);
}

int SDLRWOps::readAt(gsl::span<std::byte> dst, size_t pos)
{
	if (!fp) {
		return -1;
	}

	size_t toRead = std::min(dst.size(), static_cast<size_t>(end) - pos);

	UniqueLock lock(mutex);
	SDL_RWseek(fp, pos, SEEK_SET);
	const auto n = static_cast<int64_t>(SDL_RWread(fp, dst.data(), 1, static_cast<int>(toRead)));
	curPos = pos + std::max(n, static_cast<int64_t>(0));

	return static_cast<int>(n);
}

void SDLRWOps::close()
{
	if (fp) {
		UniqueLock lock(mutex);

		if (closeOnFinish) {
			SDL_RWclose(fp);
		}
		fp = nullptr;
		curPos = end;
	}
}

void SDLRWOps::seek(int64_t offset, int whence)
{
	UniqueLock lock(mutex);

	if (whence == SEEK_SET) {
		curPos = offset + start;
	} else if (whence == SEEK_CUR) {
		curPos += offset;
	} else if (whence == SEEK_END) {
		curPos = end + offset;
	}

	SDL_RWseek(fp, curPos, SEEK_SET);
}

size_t SDLRWOps::tell() const
{
	return static_cast<size_t>(curPos - start);
}
