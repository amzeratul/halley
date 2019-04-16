#pragma once

#include <gsl/gsl>
#include <Metal/Metal.h>

namespace Halley {
  class MetalVideo;

  class MetalBuffer {
    public:
    enum class Type
    {
      Vertex,
      Index,
      Constant
    };

    MetalBuffer(MetalVideo& video, Type type, size_t initialSize = 0);
    MetalBuffer(MetalBuffer&& other) noexcept;
    ~MetalBuffer();

    MetalBuffer(const MetalBuffer& other) = delete;

    MetalBuffer& operator=(const MetalBuffer& other) = delete;
    MetalBuffer& operator=(MetalBuffer&& other) = delete;

    void setData(gsl::span<const gsl::byte> data);
    id<MTLBuffer> getBuffer();
    size_t getOffset() const;
    size_t getLastSize() const;
    bool canFit(size_t size) const;

    void reset();
    void clear();

  private:
    MetalVideo& video;
    Type type;
    id<MTLBuffer> buffer = nil;
    size_t curSize = 0;
    size_t curPos = 0;
    size_t lastSize = 0;
    size_t lastPos = 0;
    bool waitingReset = false;

    void resize(size_t size);
  };
}
