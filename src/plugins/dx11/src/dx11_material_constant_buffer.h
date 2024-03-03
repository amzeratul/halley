#pragma once
#include "halley/graphics/material/material.h"
#include "dx11_buffer.h"

namespace Halley
{
	class DX11MaterialConstantBuffer final : public MaterialConstantBuffer
	{
	public:
		DX11MaterialConstantBuffer(DX11Video& video);

		void update(gsl::span<const gsl::byte> data) override;
		DX11Buffer& getBuffer();

	private:
		DX11Buffer buffer;
	};

	class DX11ShaderStorageBuffer final : public MaterialShaderStorageBuffer
	{
	public:
		DX11ShaderStorageBuffer(DX11Video& video);
		~DX11ShaderStorageBuffer() override;

		void update(size_t numElements, size_t pitch, gsl::span<const gsl::byte> data) override;
		void bind(ShaderType type, int position) override;

	private:
		DX11Video& video;
		DX11Buffer buffer;
		ID3D11ShaderResourceView* srv = nullptr;

		size_t numSlots = 0;

		void clearView();
	};
}
