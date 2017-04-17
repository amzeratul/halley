#pragma once

#include "halley_gl.h"
#include <gsl/gsl>

namespace Halley
{
	class GLBuffer
	{
	public:
		GLBuffer();
		~GLBuffer();

		void bind();
		void bindToTarget(GLuint index);
		void init(GLenum target);
		void setData(gsl::span<const gsl::byte> data);
		size_t getSize() const;

	private:
		GLenum target = 0;
        GLuint name = 0;
		size_t capacity = 0;
		size_t size = 0;
	};
}
