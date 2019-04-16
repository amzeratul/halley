#include "metal_buffer.h"

using namespace Halley;

MetalBuffer::MetalBuffer(MetalVideo& video, Type type, size_t initialSize)
  : video(video)
  , type(type)
{}
