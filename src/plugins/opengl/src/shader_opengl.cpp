#include <cstring>
#include "halley/support/exception.h"
#include "halley/support/console.h"
#include "shader_opengl.h"
#include "halley/core/graphics/material/material_definition.h"
#include "gl_utils.h"
#include "halley_gl.h"

using namespace Halley;

static ShaderOpenGL* currentShader = nullptr;

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

ShaderOpenGL::ShaderOpenGL(const ShaderDefinition& definition)
{
	id = glCreateProgram();
	glCheckError();	

	name = definition.name;
	setAttributes(definition.vertexAttributes);
	loadShaders(definition.shaders);
	compile();
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

static GLuint loadShader(const Bytes& src, GLenum type, String name)
{
	// Create shader
	GLuint shader = glCreateShader(type);
	glCheckError();

	// Load source
#ifdef WITH_OPENGL_ES3
    size_t startPos = 0;
    if (src.size() >= 13 && memcmp(src.data(), "#version 330", 12) == 0) {
        startPos = 12;
    }
    size_t len = src.size() + 15 - startPos;
    GLchar* buffer = new GLchar[len + 1];
    memcpy(buffer, "#version 300 es", 15);
    memcpy(buffer + 15, src.data() + startPos, len - 15);
    buffer[len] = 0;
#else
	GLchar* buffer = new GLchar[len + 1];
	memcpy(buffer, src.data(), src.size());
	buffer[len] = 0;
#endif

	// Set source
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
		String msg = String("Error compiling shader \"" + name + "\":\n") + log;
		delete[] log;
		glCheckError();
		throw Exception(msg, HalleyExceptions::VideoPlugin);
	}

	return shader;
}

void ShaderOpenGL::loadShaders(const std::map<ShaderType, Bytes>& sources)
{
	for (auto& s: sources) {
		auto type = s.first;
		int glType = 0;
		switch (type) {
		case ShaderType::Vertex:
			glType = GL_VERTEX_SHADER;
			break;
		case ShaderType::Pixel:
			glType = GL_FRAGMENT_SHADER;
			break;
#ifdef WITH_OPENGL
		case ShaderType::Geometry:
			glType = GL_GEOMETRY_SHADER;
			break;
#endif
		default:
			throw Exception("Unsupported shader type: " + toString(type), HalleyExceptions::VideoPlugin);
		}

		shaders.push_back(loadShader(s.second, glType, name + "/" + toString(type)));
	}
}

void ShaderOpenGL::compile()
{
	if (!ready) {
		// Create program
		for (size_t i = 0; i<shaders.size(); i++) {
			glAttachShader(id, shaders[i]);
			glCheckError();
		}
		glLinkProgram(id);
		glCheckError();

		// Collect log
		int infolen;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infolen);
		glCheckError();
		char* logRaw = new char[infolen + 1];
		glGetProgramInfoLog(id, infolen, &infolen, logRaw);
		logRaw[infolen] = 0;
		String log = logRaw;
		delete[] logRaw;
		glCheckError();

		// Verify result
		int result;
		glGetProgramiv(id, GL_LINK_STATUS, &result);
		glCheckError();
		if (result == GL_FALSE) {
			throw Exception("Error loading shader: " + log, HalleyExceptions::VideoPlugin);
		} else if (infolen > 0) {
			std::cout << ConsoleColour(Console::YELLOW) << "\nIn shader \"" << name << "\":\n==========\n" << log << "\n==========" << ConsoleColour() << std::endl;
		}

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

		for (size_t i = 0; i < shaders.size(); i++) {
			glDetachShader(id, shaders[i]);
			glCheckError();
			glDeleteShader(shaders[i]);
			glCheckError();
		}
	}

	if (id != 0) {
		glDeleteProgram(id);
		glCheckError();
		id = 0;
	}
}

void ShaderOpenGL::setUniformBlockBinding(unsigned int blockIndex, unsigned int binding)
{
	glUniformBlockBinding(id, blockIndex, binding);
	glCheckError();
}

int ShaderOpenGL::getUniformLocation(const String& name, ShaderType stage)
{
	if (stage != ShaderType::Combined) {
		// OpenGL doesn't support per-stage bindings
		return -1;
	}

	auto i = uniformLocations.find(name);
	if (i != uniformLocations.end()) {
		return int(i->second);
	}

	unsigned int result = glGetUniformLocation(id, name.c_str());
	glCheckError();

	uniformLocations[name] = result;
	return int(result);
}

int ShaderOpenGL::getBlockLocation(const String& name, ShaderType stage)
{
	if (stage != ShaderType::Combined) {
		// OpenGL doesn't support per-stage bindings
		return -1;
	}

	auto i = blockLocations.find(name);
	if (i != blockLocations.end()) {
		return int(i->second);
	}

	unsigned int result = glGetUniformBlockIndex(id, name.c_str());
	glCheckError();

	blockLocations[name] = result;
	return int(result);
}

int ShaderOpenGL::getAttributeLocation(const String& name)
{
	auto i = attributeLocations.find(name);
	if (i != attributeLocations.end()) {
		return int(i->second);
	}

	unsigned int result = glGetAttribLocation(id, name.c_str());
	glCheckError();

	attributeLocations[name] = result;
	return int(result);
}

void ShaderOpenGL::setAttributes(const Vector<MaterialAttribute>& attributes)
{
	for (auto& attribute : attributes) {
		glBindAttribLocation(id, attribute.location, attribute.name.c_str());
		glCheckError();
	}
}
