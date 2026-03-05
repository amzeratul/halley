#include "dx12_descriptor_pool.h"

#include "dx12_video.h"

using namespace Halley;

DX12DescriptorPool::DX12DescriptorPool(DX12Video& video, D3D12_DESCRIPTOR_HEAP_TYPE type, size_t size)
        : video(video)
{
    size = alignUp<size_t>(size, 64);

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = type;
    desc.NumDescriptors = UINT(size);

    bool isShaderVisible = type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
            type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

    if (isShaderVisible) {
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }

    if (FAILED(video.getDevice().CreateDescriptorHeap(&desc, D3D12_IID_ARGS(heap)))) {
        throw Exception("Failed to create descriptor heap", HalleyExceptions::VideoPlugin);
    }

    cpu = heap->GetCPUDescriptorHandleForHeapStart();

    if (isShaderVisible) {
        gpu = heap->GetGPUDescriptorHandleForHeapStart();
    }

    heapIncrementSize = video.getDevice().GetDescriptorHandleIncrementSize(type);

    regions.resize(size / 64);
}

DX12DescriptorPool::~DX12DescriptorPool()
{
    size_t numInUse = 0;
    for (const auto& region : regions) {
        numInUse += countBits(region.mask);
    }

    if (numInUse > 0) {
        // TODO: "leaks", add warning
    }

    heap.Reset();
}

DX12DescriptorHandle DX12DescriptorPool::alloc(size_t count)
{
    HalleyAssertDev(count > 0 && count <= 64);
    DX12DescriptorHandle handle = InvalidDescriptorHandle;

    for (size_t i = 0; i < regions.size() && handle == InvalidDescriptorHandle; i++) {
        auto& region = regions[i];
        uint64_t bits = region.mask;
        size_t shift = 0;
        do {
            // Count number of trailing zero-bits
            size_t lsb = leastSignificantBit(bits);
            if (lsb >= count) {
                // Found slice of free blocks, mark as used
                uint64_t mask = ((1ull << count) - 1ull) << shift;
                region.mask |= mask;
                // Store allocation size in first block
                region.refs[shift] = uint8_t(count);
                // Handle is the index into the whole table
                handle = i * 64 + shift;
                break;
            }
            // Flip trailing zero bits
            bits |= (1ull << lsb) - 1ull;
            // Now count number of trailing one-bits
            lsb = leastSignificantBit(~bits);
            // Rotate (roll over) all one-bits; eventually the mask will run
            // out of zero-bits and we are done
            bits = rotateRight(bits, lsb);
            shift += lsb;
        } while (bits != ~0ull);
    }

    HalleyAssertDev(handle != InvalidDescriptorHandle);

    countInUse += count;

    return handle;
}

void DX12DescriptorPool::free(DX12DescriptorHandle handle)
{
    auto& region = regions[handle / 64];
    size_t shift = handle % 64;

    // Recover allocation size, and clear the stored one
    size_t count = region.refs[shift];
    HalleyAssertDev((count > 0) && (shift + count <= 64));
    region.refs[shift] = 0;

    // Update mask to mark blocks as free
    uint64_t mask = ((1ull << count) - 1ull) << shift;
    region.mask &= ~mask;

    HalleyAssertDev(countInUse >= count);

    countInUse -= count;
}

size_t DX12DescriptorPool::getUsagePercent() const
{
    return countInUse * 100 / (regions.size() * 64);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12DescriptorPool::getCPUHandle(DX12DescriptorHandle handle, size_t offset) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE out;
    out.ptr = cpu.ptr + (handle + offset) * heapIncrementSize;
    return out;
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12DescriptorPool::getGPUHandle(DX12DescriptorHandle handle, size_t offset) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE out;
    out.ptr = gpu.ptr + (handle + offset) * heapIncrementSize;
    return out;
}
