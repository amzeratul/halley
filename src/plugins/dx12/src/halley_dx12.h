#pragma once

#ifdef DEV_BUILD
#define D3D12_DEBUG 0
#else
#define D3D12_DEBUG 0
#endif

#ifdef _GAMING_XBOX
#if defined(_GAMING_XBOX_XBOXONE)
#include <d3d12_x.h>
#elif defined(_GAMING_XBOX_SCARLETT)
#include <d3d12_xs.h>
#endif
#include <XSystem.h>
#else
#include <d3d12.h>

#ifdef _GAMING_DESKTOP
DECLARE_HANDLE(HMONITOR); // TODO: fix me!
#endif

#include <dxgi1_6.h>

#if D3D12_DEBUG
#include <dxgidebug.h>
#endif
#endif

#include <wrl.h>
using namespace Microsoft::WRL;

#ifdef _GAMING_XBOX
#define D3D12_IID_ARGS(ppType) IID_GRAPHICS_PPV_ARGS(ppType.ReleaseAndGetAddressOf())
#else
#define D3D12_IID_ARGS(ppType) IID_PPV_ARGS(&ppType)
#endif

namespace Halley {
    using DX12DescriptorHandle = size_t;
    constexpr DX12DescriptorHandle InvalidDescriptorHandle = ~0ull;
}
