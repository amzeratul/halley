#pragma once
#include "halley/concurrency/executor.h"
#include "halley/core/api/system_api.h"

namespace Halley {
	class DX11Video;

	class DX11Loader
    {
    public:
		DX11Loader(DX11Video& video);
		~DX11Loader();

	private:
		Executor executor;
		std::thread thread;
	};
}
