#pragma once
#include <gsl/gsl>
#include <d3d11.h>
#undef min
#undef max

namespace Halley {
	class DX11Video;

	class DX11Buffer {
    public:
        DX11Buffer(DX11Video& video);
		~DX11Buffer();

		void setData(gsl::span<const gsl::byte> data);
		ID3D11Buffer*& getBuffer();

	private:
		DX11Video& video;
		ID3D11Buffer* buffer = nullptr;
		size_t curSize = 0;

		void resize(size_t size);
	};
}
