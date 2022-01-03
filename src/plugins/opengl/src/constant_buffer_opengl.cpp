#include "constant_buffer_opengl.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/graphics/material/material_definition.h"
#include "gl_utils.h"
#include "halley/support/exception.h"
#include "texture_opengl.h"

using namespace Halley;

ConstantBufferOpenGL::ConstantBufferOpenGL()
{
	buffer.init(GL_UNIFORM_BUFFER);
}

ConstantBufferOpenGL::~ConstantBufferOpenGL()
{
	
}

void ConstantBufferOpenGL::update(gsl::span<const gsl::byte> data)
{
	buffer.setData(data);
}

void ConstantBufferOpenGL::bind(int bindPoint)
{
	buffer.bindToTarget(bindPoint);
	glCheckError();
}
