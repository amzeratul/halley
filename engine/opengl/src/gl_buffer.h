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
		void init(GLenum target);
		void setData(gsl::span<const gsl::byte> data);
		
	private:
        GLuint name = 0;
		GLenum target = 0;
		size_t size = 0;
	};
}
