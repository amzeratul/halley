#include "shader_opengl.h"
#include "gl_utils.h"

using namespace Halley;

static ShaderOpenGL* currentShader = nullptr;

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

ShaderOpenGL::ShaderOpenGL(String name)
	: Shader(name)
{
}

ShaderOpenGL::~ShaderOpenGL()
{
	destroy();
}

void ShaderOpenGL::bind()
{
	if (this != currentShader) {
		if (!ready) {
			compile();
		}
		glUseProgram(id);
		glCheckError();
		currentShader = this;
	}
}

void ShaderOpenGL::unbind()
{
	currentShader = nullptr;
	glUseProgram(0);
	glCheckError();
}

static GLuint loadShader(String src, GLenum type)
{
	// Create shader
	GLuint shader = glCreateShader(type);
	glCheckError();

	// Load source
	size_t len = src.size();
	GLchar* buffer = new GLchar[len + 1];
	strcpy(buffer, src.c_str());
	buffer[len] = 0;
	const char* cbuf = buffer;
	glShaderSource(shader, 1, &cbuf, nullptr);
	glCheckError();

	// Compile source
	glCompileShader(shader);
	glCheckError();
	delete[] buffer;

	// Check result
	// Seriously, GL? All this crap to retrieve an info log?
	int result;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	glCheckError();
	if (result == GL_FALSE) {
		int infolen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infolen);
		glCheckError();
		char* log = new char[infolen];
		glGetShaderInfoLog(shader, infolen, &infolen, log);
		String msg = String("Error compiling shader: ") + log;
		delete[] log;
		glCheckError();
		throw Exception(msg);
	}

	return shader;
}

void ShaderOpenGL::compile()
{
	if (!ready) {
		// Compile shaders
		for (size_t i = 0; i<vertexSources.size(); i++) {
			shaders.push_back(loadShader(vertexSources[i], GL_VERTEX_SHADER));
		}
#ifndef WITH_OPENGL_ES
		for (size_t i = 0; i<geometrySources.size(); i++) {
			shaders.push_back(loadShader(geometrySources[i], GL_GEOMETRY_SHADER_ARB));
		}
#endif
		for (size_t i = 0; i<pixelSources.size(); i++) {
			shaders.push_back(loadShader(pixelSources[i], GL_FRAGMENT_SHADER));
		}

		// Create program
		GLuint program = glCreateProgram();
		glCheckError();
		for (size_t i = 0; i<shaders.size(); i++) {
			glAttachShader(program, shaders[i]);
			glCheckError();
		}
		glLinkProgram(program);
		glCheckError();

		// Verify result
		int result;
		glGetProgramiv(program, GL_LINK_STATUS, &result);
		glCheckError();
		if (result == GL_FALSE) {
			int infolen;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infolen);
			glCheckError();
			char* log = new char[infolen];
			glGetProgramInfoLog(program, infolen, &infolen, log);
			String msg = String("Error loading shader: ") + log;
			delete[] log;
			glCheckError();
			throw Exception(msg);
		}

		id = program;
		uniformLocations.clear();
		attributeLocations.clear();
		ready = true;
	}
}

void ShaderOpenGL::destroy()
{
	if (ready) {
		glCheckError();
		unbind();
		ready = false;

		for (size_t i = 0; i<shaders.size(); i++) {
			glDetachShader(id, shaders[i]);
			glCheckError();
			glDeleteShader(shaders[i]);
			glCheckError();
		}
		glDeleteProgram(id);
		glCheckError();
	}
}

void ShaderOpenGL::addVertexSource(String src)
{
	vertexSources.emplace_back(src);
}

void ShaderOpenGL::addGeometrySource(String src)
{
	geometrySources.emplace_back(src);
}

void ShaderOpenGL::addPixelSource(String src)
{
	pixelSources.emplace_back(src);
}

unsigned ShaderOpenGL::getUniformLocation(String name)
{
	auto i = uniformLocations.find(name);
	if (i != uniformLocations.end()) {
		return i->second;
	}

	unsigned int result = glGetUniformLocation(id, name.c_str());
	glCheckError();
	if (result == static_cast<unsigned int>(-1)) {
		throw Exception("Invalid shader uniform name: \"" + String(name) + "\"");
	}
	uniformLocations[name] = result;
	return result;
}

unsigned ShaderOpenGL::getAttributeLocation(String name)
{
	auto i = attributeLocations.find(name);
	if (i != attributeLocations.end()) {
		return i->second;
	}

	unsigned int result = glGetAttribLocation(id, name.c_str());
	glCheckError();
	if (result == static_cast<unsigned int>(-1)) {
		throw Exception("Invalid shader attribute name: \"" + name + "\" in shader \"" + this->name + "\"");
	}
	attributeLocations[name] = result;
	return result;
}
