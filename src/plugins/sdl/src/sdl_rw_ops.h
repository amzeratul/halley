#pragma once
#include <cstdint>
#include <memory>
#include <SDL.h>
#include "halley/resources/resource_data.h"

namespace Halley
{
	class SDLRWOps : public ResourceDataReader
	{
	public:
		static std::unique_ptr<ResourceDataReader> fromPath(const String& path, int64_t start, int64_t end);
		static std::unique_ptr<ResourceDataReader> fromMemory(gsl::span<const std::byte> span);

		SDLRWOps(SDL_RWops* fp, int64_t start, int64_t end, bool closeOnFinish);
		~SDLRWOps();

		size_t size() const override;
		int read(gsl::span<std::byte> dst) override;
		int readAt(gsl::span<std::byte> dst, size_t pos) override;
		void seek(int64_t pos, int whence) override;
		size_t tell() const override;
		void close() override;

	private:
		SDL_RWops* fp;
		std::atomic<int64_t> curPos;
		int64_t start;
		int64_t end;
		bool closeOnFinish;
		Mutex mutex;
	};
}
