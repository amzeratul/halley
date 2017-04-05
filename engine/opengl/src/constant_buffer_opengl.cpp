#include "constant_buffer_opengl.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/graphics/material/material_definition.h"
#include "gl_utils.h"
#include "halley/support/exception.h"
#include "texture_opengl.h"

using namespace Halley;

ConstantBufferOpenGL::ConstantBufferOpenGL(const Material& material)
	: material(material)
{
}

void ConstantBufferOpenGL::update(const Material& material)
{
	
}

void ConstantBufferOpenGL::bind(int pass)
{
	for (auto& u: material.getUniforms()) {
		ShaderParameterType type = u.getType();
		int loc = u.getAddress(pass);
		auto data = u.getData();
		auto ints = reinterpret_cast<const int*>(data);
		auto floats = reinterpret_cast<const float*>(data);

		switch (type) {
		case ShaderParameterType::Float:
			glUniform1f(loc, floats[0]);
			break;
		case ShaderParameterType::Float2:
			glUniform2f(loc, floats[0], floats[1]);
			break;
		case ShaderParameterType::Float3:
			glUniform3f(loc, floats[0], floats[1], floats[2]);
			break;
		case ShaderParameterType::Float4:
			glUniform4f(loc, floats[0], floats[1], floats[2], floats[3]);
			break;
		case ShaderParameterType::Int:
			glUniform1i(loc, ints[0]);
			break;
		case ShaderParameterType::Int2:
			glUniform2i(loc, ints[0], ints[1]);
			break;
		case ShaderParameterType::Int3:
			glUniform3i(loc, ints[0], ints[1], ints[2]);
			break;
		case ShaderParameterType::Int4:
			glUniform4i(loc, ints[0], ints[1], ints[2], ints[3]);
			break;
		case ShaderParameterType::Texture2D:
			{
				int textureUnit = ints[0];
				glUniform1i(loc, textureUnit);
				auto texture = std::static_pointer_cast<const TextureOpenGL>(material.getTexture(textureUnit));
				if (!texture) {
					//throw Exception("Error binding texture to texture unit #" + toString(textureUnit) + " with material \"" + material.getDefinition().getName() + "\": texture is null.");					
				} else {
					texture->bind(textureUnit);
				}
			}
			break;
		case ShaderParameterType::Matrix2:
			glUniformMatrix2fv(loc, 1, false, floats);
			break;
		case ShaderParameterType::Matrix3:
			glUniformMatrix3fv(loc, 1, false, floats);
			break;
		case ShaderParameterType::Matrix4:
			glUniformMatrix4fv(loc, 1, false, floats);
			break;
		case ShaderParameterType::Invalid:
		default:
			throw Exception("Invalid parameter type");
		}
	}
	glCheckError();
}
