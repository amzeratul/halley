#include "sdl_rw_ops.h"

using namespace Halley;

std::unique_ptr<ResourceDataReader> SDLRWOps::fromPath(const String& path, int64_t start, int64_t end)
{
	auto fp = SDL_RWFromFile(path.c_str(), "rb");
	if (!fp) {
		return std::unique_ptr<ResourceDataReader>();
	}
	return std::make_unique<SDLRWOps>(fp, start, end, true);
}

std::unique_ptr<ResourceDataReader> SDLRWOps::fromMemory(gsl::span<const gsl::byte> span)
{
	auto fp = SDL_RWFromConstMem(span.data(), int(span.size()));
	if (!fp) {
		return std::unique_ptr<ResourceDataReader>();
	}
	return std::make_unique<SDLRWOps>(fp, 0, 0, true);
}

SDLRWOps::SDLRWOps(SDL_RWops* _fp, int64_t _start, int64_t _end, bool _closeOnFinish)
	: fp(_fp)
	, pos(_start)
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
		throw Exception("Invalid file size for resource: " + toString(size) + " bytes.");
	}
}

size_t SDLRWOps::size() const
{
	return end - start;
}

int SDLRWOps::read(gsl::span<gsl::byte> dst)
{
	if (!fp) return -1;

	size_t toRead = std::min(size_t(dst.size()), size_t(end - pos));
	SDL_RWseek(fp, pos, SEEK_SET);
	int n = static_cast<int>(SDL_RWread(fp, dst.data(), 1, static_cast<int>(toRead)));
	if (n > 0) pos += n;
	return n;
}

void SDLRWOps::close()
{
	if (fp) {
		if (closeOnFinish) {
			SDL_RWclose(fp);
		}
		fp = nullptr;
		pos = end;
	}
}

void SDLRWOps::seek(int64_t offset, int whence)
{
	if (whence == SEEK_SET) pos = int(offset + start);
	else if (whence == SEEK_CUR) pos += int(offset);
	else if (whence == SEEK_END) pos = int(end + offset);
	SDL_RWseek(fp, pos, SEEK_SET);
}

size_t SDLRWOps::tell() const
{
	return pos - start;
}
