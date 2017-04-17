#include "halley_gl.h"
#include "video_opengl.h"
#include "painter_opengl.h"
#include "texture_opengl.h"
#include "shader_opengl.h"
#include "render_target_opengl.h"
#include <halley/support/console.h>
#include <halley/support/exception.h>
#include <halley/support/debug.h>
#include <halley/core/graphics/window.h>
#include "halley/text/string_converter.h"
#include "constant_buffer_opengl.h"
using namespace Halley;

#ifdef _MSC_VER
	#if defined(WITH_OPENGL)
		#pragma comment(lib, "opengl32.lib")
	#elif defined(WITH_OPENGL_ES2)
		#ifdef _DEBUG
			#pragma comment(lib, "libEGL_d.lib")
			#pragma comment(lib, "libGLESv2_d.lib")
		#else
			#pragma comment(lib, "libEGL.lib")
			#pragma comment(lib, "libGLESv2.lib")
		#endif
	#elif defined(WITH_OPENGL_ES)
		#pragma error("GLES 1.0 is not supported with MSVC")
	#endif
#endif

void VideoOpenGL::init()
{
	setUpEnumMap();
}

void VideoOpenGL::deInit()
{
	loaderThread.reset();

	context.reset();
	system.destroyWindow(window);
	window.reset();

	std::cout << "Video terminated." << std::endl;
}


///////////////
// Constructor
VideoOpenGL::VideoOpenGL(SystemAPI& system)
	: system(system)
	, initialized(false)
{
}


/////////////
// Set video
void VideoOpenGL::setWindow(WindowDefinition&& windowDefinition, bool vsync)
{
	// Initialize video mode
	if (!initialized) {
		window = system.createWindow(windowDefinition);
		initOpenGL();
		window->setVsync(vsync);

		clearScreen();
		window->show();
		flip();
		startLoaderThread();
		initialized = true;
	} else {
		window->update(windowDefinition);
		clearScreen();
	}
}

const Window& VideoOpenGL::getWindow() const
{
	return *window;
}

void VideoOpenGL::initOpenGL()
{
	std::cout << ConsoleColour(Console::GREEN) << "Initializing OpenGL...\n" << ConsoleColour() << std::endl;

	// Create OpenGL context
	context = system.createGLContext();
	context->bind();
	
	initGLBindings();

	// Print OpenGL data
	std::cout << "OpenGL initialized with:" << std::endl;
	std::cout << "\tVersion: " << ConsoleColour(Console::DARK_GREY) << glGetString(GL_VERSION) << ConsoleColour() << std::endl;
	std::cout << "\tVendor: " << ConsoleColour(Console::DARK_GREY) << glGetString(GL_VENDOR) << ConsoleColour() << std::endl;
	std::cout << "\tRenderer: " << ConsoleColour(Console::DARK_GREY) << glGetString(GL_RENDERER) << ConsoleColour() << std::endl;
	std::cout << "\tGLSL Version: " << ConsoleColour(Console::DARK_GREY) << glGetString(GL_SHADING_LANGUAGE_VERSION) << ConsoleColour() << std::endl;

	// Print extensions
#ifndef _DEBUG
	std::cout << "\tExtensions: " << ConsoleColour(Console::DARK_GREY);
#ifdef WITH_OPENGL
	int nExtensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &nExtensions);
	for (int i = 0; i < nExtensions; i++) {
		String str = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
		std::cout << str << " ";
	}
#else
	String str = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
#endif
	std::cout << ConsoleColour() << std::endl;
#endif

	setupDebugCallback();

	std::cout << ConsoleColour(Console::GREEN) << "OpenGL init done.\n" << ConsoleColour() << std::endl;
}

void VideoOpenGL::initGLBindings()
{
#ifdef WITH_OPENGL
	if (ogl_LoadFunctions() == ogl_LOAD_FAILED) {
		throw Exception(String("Error initializing glLoadGen."));
	}
	glCheckError();
#endif
}

void VideoOpenGL::startLoaderThread()
{
	loaderThread = std::make_unique<LoaderThreadOpenGL>(*context);
}

void VideoOpenGL::setupDebugCallback()
{
#ifdef WITH_OPENGL
	if (glDebugMessageCallback) {
#ifdef _DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif
		glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
			reinterpret_cast<const VideoOpenGL*>(userParam)->onGLDebugMessage(source, type, id, severity, message);
		}, this);
		glCheckError();
	} else {
		glGetError();
		std::cout << ConsoleColour(Console::YELLOW) << "KHR_DEBUG is not available." << ConsoleColour() << std::endl;
	}
#endif
}

void VideoOpenGL::onSuspend()
{
#ifdef WITH_OPENGL
	if (glDebugMessageCallback) {
		glDebugMessageCallback(nullptr, nullptr);
		glCheckError();
	}
	loaderThread.reset();
#endif
}

void VideoOpenGL::onResume()
{
	context->bind();
	initGLBindings();
	setupDebugCallback();
	startLoaderThread();
}

void VideoOpenGL::clearScreen()
{
	// Clear buffer
	glCheckError();
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_CULL_FACE);
	glCheckError();

	// Swap buffers
	flip();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	flip();
}

void VideoOpenGL::setUpEnumMap()
{
#ifdef WITH_OPENGL
	glEnumMap[GL_DEBUG_SOURCE_API] = "API";
	glEnumMap[GL_DEBUG_SOURCE_WINDOW_SYSTEM] = "Window System";
	glEnumMap[GL_DEBUG_SOURCE_SHADER_COMPILER] = "Shader Compiler";
	glEnumMap[GL_DEBUG_SOURCE_THIRD_PARTY] = "Third Party";
	glEnumMap[GL_DEBUG_SOURCE_APPLICATION] = "Application";
	glEnumMap[GL_DEBUG_SOURCE_OTHER] = "Other";
	glEnumMap[GL_DEBUG_TYPE_ERROR] = "Error";
	glEnumMap[GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR] = "Deprecated Behaviour";
	glEnumMap[GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR] = "Undefined Behaviour";
	glEnumMap[GL_DEBUG_TYPE_PORTABILITY] = "Portability";
	glEnumMap[GL_DEBUG_TYPE_PERFORMANCE] = "Performance";
	glEnumMap[GL_DEBUG_TYPE_MARKER] = "Marker";
	glEnumMap[GL_DEBUG_TYPE_PUSH_GROUP] = "Push Group";
	glEnumMap[GL_DEBUG_TYPE_POP_GROUP] = "Pop Group";
	glEnumMap[GL_DEBUG_TYPE_OTHER] = "Other";
	glEnumMap[GL_DEBUG_SEVERITY_HIGH] = "High";
	glEnumMap[GL_DEBUG_SEVERITY_MEDIUM] = "Medium";
	glEnumMap[GL_DEBUG_SEVERITY_LOW] = "Low";
	glEnumMap[GL_DEBUG_SEVERITY_NOTIFICATION] = "Notification";
#endif
}

void VideoOpenGL::onGLDebugMessage(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, String message) const
{
#ifdef WITH_OPENGL
	if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_LOW) {
		std::stringstream ss;
#if HAS_EASTL
		ss << "[" << glEnumMap.at(source).second << "] [" << glEnumMap.at(type).second << "] [" << glEnumMap.at(severity).second << "] " << id << ": " << message;
#else
		ss << "[" << glEnumMap.at(source) << "] [" << glEnumMap.at(type) << "] [" << glEnumMap.at(severity) << "] " << id << ": " << message;
#endif
		std::string str = ss.str();

#ifdef _DEBUG
		std::cout << ConsoleColour(Console::YELLOW) << str << ConsoleColour() << std::endl;
#else
		std::lock_guard<std::mutex> lock(messagesMutex);
		messagesPending.push_back([str] () {
			std::cout << ConsoleColour(Console::YELLOW) << str << ConsoleColour() << std::endl;
		});
#endif
	}
#endif
}

static std::function<void(int, void*)> getUniformBinding(UniformType type, int n)
{
	switch (type) {
		case UniformType::Int:
		{
			if (n == 1) {
				return [](int address, void* data)
				{
					glUniform1i(address, reinterpret_cast<int*>(data)[0]);
					glCheckError();
				};
			} else if (n == 2) {
				return [](int address, void* data)
				{
					auto vs = reinterpret_cast<int*>(data);
					glUniform2i(address, vs[0], vs[1]);
					glCheckError();
				};
			} else if (n == 3) {
				return [](int address, void* data)
				{
					auto vs = reinterpret_cast<int*>(data);
					glUniform3i(address, vs[0], vs[1], vs[2]);
					glCheckError();
				};
			} else if (n == 4) {
				return [](int address, void* data)
				{
					auto vs = reinterpret_cast<int*>(data);
					glUniform4i(address, vs[0], vs[1], vs[2], vs[3]);
					glCheckError();
				};
			}
		}
		case UniformType::IntArray:
		{
			return [n](int address, void* data)
			{
				auto vs = reinterpret_cast<int*>(data);
				glUniform1iv(address, n, vs);
				glCheckError();
			};
		}
		case UniformType::Float:
		{
			if (n == 1) {
				return [](int address, void* data)
				{
					glUniform1f(address, reinterpret_cast<float*>(data)[0]);
					glCheckError();
				};
			}
			else if (n == 2) {
				return [](int address, void* data)
				{
					auto vs = reinterpret_cast<float*>(data);
					glUniform2f(address, vs[0], vs[1]);
					glCheckError();
				};
			}
			else if (n == 3) {
				return [](int address, void* data)
				{
					auto vs = reinterpret_cast<float*>(data);
					glUniform3f(address, vs[0], vs[1], vs[2]);
					glCheckError();
				};
			}
			else if (n == 4) {
				return [](int address, void* data)
				{
					auto vs = reinterpret_cast<float*>(data);
					glUniform4f(address, vs[0], vs[1], vs[2], vs[3]);
					glCheckError();
				};
			}
		}
		case UniformType::FloatArray:
		{
			return [=](int address, void* data)
			{
				auto vs = reinterpret_cast<float*>(data);
				glUniform1fv(address, n, vs);
				glCheckError();
			};
		}
		case UniformType::Mat4:
		{
			return [](int address, void* data)
			{
				auto vs = reinterpret_cast<Matrix4f*>(data);
				glUniformMatrix4fv(address, 1, false, vs->getElements());
				glCheckError();
			};
		}
		default:
			throw Exception("Unsupported uniform type: " + toString(static_cast<int>(type)));
	}
}

std::unique_ptr<MaterialConstantBuffer> VideoOpenGL::createConstantBuffer()
{
	return std::make_unique<ConstantBufferOpenGL>();
}

std::unique_ptr<Painter> VideoOpenGL::makePainter(Resources& resources)
{
	return std::make_unique<PainterOpenGL>(resources);
}

std::unique_ptr<Texture> VideoOpenGL::createTexture(Vector2i size)
{
	return std::make_unique<TextureOpenGL>(*this, size);
}

std::unique_ptr<Shader> VideoOpenGL::createShader(const ShaderDefinition& definition)
{
	return std::make_unique<ShaderOpenGL>(definition);
}

std::unique_ptr<TextureRenderTarget> VideoOpenGL::createRenderTarget()
{
	return std::make_unique<RenderTargetOpenGL>();
}

bool VideoOpenGL::isLoaderThread() const
{
	return loaderThread && std::this_thread::get_id() == loaderThread->getThreadId();
}

void VideoOpenGL::startRender()
{
	HALLEY_DEBUG_TRACE();

	// TODO
	/*
	if (!TextureLoadQueue::hasLoaderThread()) {
		Debug::trace("Game::RenderScreen loading texture");
		TextureLoadQueue::get()->load(1);
		Debug::trace("Game::RenderScreen loaded texture");
	}
	*/
}

void VideoOpenGL::finishRender()
{
	HALLEY_DEBUG_TRACE();
	flip();
	HALLEY_DEBUG_TRACE();

	glCheckError();
}

void VideoOpenGL::flip()
{
	window->swap();

	Vector<std::function<void()>> msgs;
	{
		std::lock_guard<std::mutex> lock(messagesMutex);
		msgs = std::move(messagesPending);
	}
	for (const auto& m : msgs) {
		m();
	}
}
