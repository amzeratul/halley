#pragma once

#include "halley_dx12.h"

namespace Halley {

    class DX12Video;

    class DX12DescriptorPool
    {
    public:
        explicit DX12DescriptorPool(DX12Video& video, D3D12_DESCRIPTOR_HEAP_TYPE type, size_t size);
        ~DX12DescriptorPool();

        DX12DescriptorHandle alloc(size_t count);
        void free(DX12DescriptorHandle handle);

        size_t getUsagePercent() const;

        ID3D12DescriptorHeap* getHeap() const { return heap.Get(); }

        D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(DX12DescriptorHandle handle, size_t offset = 0) const;
        D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(DX12DescriptorHandle handle, size_t offset = 0) const;

    private:
        DX12Video& video;
        ComPtr<ID3D12DescriptorHeap> heap;
        size_t heapIncrementSize;

        D3D12_CPU_DESCRIPTOR_HANDLE cpu = {};
        D3D12_GPU_DESCRIPTOR_HANDLE gpu = {};

        struct Region
        {
            uint64_t mask = 0;
            Vector<uint8_t> refs = Vector<uint8_t>(64);
        };

        Vector<Region> regions;
        size_t countInUse = 0;
    };

}
