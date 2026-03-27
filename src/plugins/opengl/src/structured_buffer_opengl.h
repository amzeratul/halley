#pragma once
#include "halley/api/video_api.h"
#include "gl_buffer.h"

namespace Halley
{
	class StructuredBufferOpenGL : public MaterialStructuredBuffer
	{
	public:
		StructuredBufferOpenGL();
		~StructuredBufferOpenGL();

		void update(gsl::span<const std::byte> data, size_t elementStride) override;
		size_t getSize() const override { return dataSize; }
		size_t getStride() const override { return stride; }

		void bind(int bindPoint);

	private:
		GLBuffer buffer;
		size_t dataSize = 0;
		size_t stride = 0;
	};
}
