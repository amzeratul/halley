#pragma once
#include <d3d11.h>
#undef min
#undef max

namespace Halley
{
	enum class BlendType;
	class DX11Video;

	class DX11Blend
	{
	public:
		DX11Blend(DX11Video& video, BlendType blend);
		DX11Blend(DX11Blend&& other) noexcept;
		~DX11Blend();

		void bind(DX11Video& video);
		DX11Blend& operator=(DX11Blend&& other) noexcept;

	private:
		ID3D11BlendState* state = nullptr;
	};
}
