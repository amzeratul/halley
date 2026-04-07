#pragma once
#include <gsl/gsl>
#include <d3d11.h>

namespace Halley {
	class DX11Video;

	class DX11Buffer {
    public:
		enum class Type
		{
			Vertex,
			Index,
			Constant,
			Structured
		};

        DX11Buffer(DX11Video& video, Type type, size_t initialSize = 0);
		DX11Buffer(DX11Buffer&& other) noexcept;
		~DX11Buffer();

		DX11Buffer(const DX11Buffer& other) = delete;

		DX11Buffer& operator=(const DX11Buffer& other) = delete;
		DX11Buffer& operator=(DX11Buffer&& other) = delete;

		void setData(gsl::span<const std::byte> data, size_t structStride = 0);
		ID3D11Buffer*& getBuffer();
		ID3D11ShaderResourceView* getSRV() const { return srv; }
		UINT getOffset() const;
		UINT getLastSize() const;
		bool canFit(size_t size) const;

		void reset();
		void clear();

	private:
		DX11Video& video;
		Type type;
		ID3D11Buffer* buffer = nullptr;
		ID3D11ShaderResourceView* srv = nullptr;
		size_t curSize = 0;
		size_t curStride = 0;
		size_t curPos = 0;
		size_t lastSize = 0;
		size_t lastPos = 0;
		bool waitingReset = false;

		void resize(size_t size, size_t structStride = 0);
	};
}
