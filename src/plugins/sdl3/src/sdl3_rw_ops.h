#pragma once

#include <SDL3/SDL.h>
#include "halley/resources/resource_data.h"

namespace Halley
{
	class SDL3RWOps : public ResourceDataReader
	{
	public:
		static std::unique_ptr<ResourceDataReader> fromPath(const String& path, int64_t start, int64_t end);
		static std::unique_ptr<ResourceDataReader> fromMemory(gsl::span<const std::byte> span);

		SDL3RWOps(SDL_IOStream* fp, int64_t start, int64_t end, bool closeOnFinish);
		~SDL3RWOps() override;

		size_t size() const override;
		int read(gsl::span<std::byte> dst) override;
		void seek(int64_t pos, int whence) override;
		size_t tell() const override;
		void close() override;

	private:
		SDL_IOStream* fp;
		int64_t pos;
		int64_t start;
		int64_t end;
		bool closeOnFinish;
	};
}
