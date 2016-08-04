#include <SDL.h>
#include "gl_core_3_3.h"
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
using namespace Halley;

#ifdef _MSC_VER
#pragma comment(lib, "opengl32.lib")
#endif

void VideoOpenGL::init()
{
	setUpEnumMap();
}

void VideoOpenGL::deInit()
{
	if (running) {
		running = false;
		//loaderThread.join();
	}

	SDL_GL_DeleteContext(context);
	SDL_GL_MakeCurrent(sdlWindow, nullptr);
	SDL_DestroyWindow(sdlWindow);
	SDL_VideoQuit();

	context = nullptr;
	sdlWindow = nullptr;

	std::cout << "Video terminated." << std::endl;
}


///////////////
// Constructor
VideoOpenGL::VideoOpenGL()
	: initialized(false)
	, running(false)
{
}


/////////////
// Set video
void VideoOpenGL::setWindow(Window&& window, bool vsync)
{
	// Initialize video mode
	if (!initialized) {
		SDL_VideoInit(nullptr);
		printDebugInfo();
		createWindow(window);
		initOpenGL(vsync);

		initialized = true;
		std::cout << ConsoleColor(Console::GREEN) << "Video init done.\n" << ConsoleColor() << std::endl;
	} else {
		updateWindow(window);
	}

	clearScreen();
	SDL_ShowWindow(sdlWindow);
}

const Window& VideoOpenGL::getWindow() const
{
	return *curWindow;
}

void VideoOpenGL::printDebugInfo() const
{
	std::cout << std::endl << ConsoleColor(Console::GREEN) << "Initializing Video Display...\n" << ConsoleColor();
	std::cout << "Drivers available:\n";
	for (int i = 0; i < SDL_GetNumVideoDrivers(); i++) {
		std::cout << "\t" << i << ": " << SDL_GetVideoDriver(i) << "\n";
	}
	std::cout << "Video driver: " << ConsoleColor(Console::DARK_GREY) << SDL_GetCurrentVideoDriver() << ConsoleColor() << std::endl;
}

void VideoOpenGL::createWindow(const Window& window)
{
	// Set flags and GL attributes
	auto windowType = window.getWindowType();
	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS;
	if (windowType == WindowType::BorderlessWindow) {
		flags |= SDL_WINDOW_BORDERLESS;
	} else if (windowType == WindowType::ResizableWindow) {
		flags |= SDL_WINDOW_RESIZABLE;
	} else if (windowType == WindowType::Fullscreen) {
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	// Context options
#if defined(WITH_OPENGL_ES2)
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 0);
#else
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#ifdef _DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

	// Window position
	Vector2i windowSize = window.getSize();
	Vector2i winPos = window.getPosition();

	// Create window
	sdlWindow = SDL_CreateWindow(window.getTitle().c_str(), winPos.x, winPos.y, windowSize.x, windowSize.y, flags);
	if (!sdlWindow) {
		throw Exception(String("Error creating SDL window: ") + SDL_GetError());
	}
	updateWindow(window);
}

void VideoOpenGL::updateWindow(const Window& window)
{
#ifdef __ANDROID__
	return;
#endif

	// Update window position & size
	// For windowed, get out of fullscreen first, then set size.
	// For fullscreen, set size before going to fullscreen.
	WindowType windowType = window.getWindowType();
	Vector2i windowPos = window.getPosition();
	Vector2i windowSize = window.getSize();

	if (windowType != WindowType::Fullscreen) {
		SDL_SetWindowFullscreen(sdlWindow, SDL_FALSE);
	}
	SDL_SetWindowSize(sdlWindow, windowSize.x, windowSize.y);
	if (windowType == WindowType::Fullscreen) {
		SDL_SetWindowFullscreen(sdlWindow, SDL_TRUE);
	}

	SDL_SetWindowPosition(sdlWindow, windowPos.x, windowPos.y);

	curWindow = std::make_unique<Window>(window);
}

void VideoOpenGL::initOpenGL(bool vsync)
{
	// Create OpenGL context
	SDL_GLContext context = SDL_GL_CreateContext(sdlWindow);
	if (!context) {
		throw Exception(String("Error creating OpenGL context: ") + SDL_GetError());
	}
	if (SDL_GL_MakeCurrent(sdlWindow, context) < 0) {
		throw Exception(String("Error setting OpenGL context: ") + SDL_GetError());
	}
	
	// Start loader thread
	if (!running) {
#ifdef ____WIN32__
		vid->loaderThread = TextureLoadQueue::startLoaderThread(window, &vid->running);
		vid->running = true;
#endif
	}

	initGLBindings();

	// Print OpenGL data
	std::cout << "OpenGL initialized." << std::endl;
	std::cout << "\tVersion: " << ConsoleColor(Console::DARK_GREY) << glGetString(GL_VERSION) << ConsoleColor() << std::endl;
	std::cout << "\tVendor: " << ConsoleColor(Console::DARK_GREY) << glGetString(GL_VENDOR) << ConsoleColor() << std::endl;
	std::cout << "\tRenderer: " << ConsoleColor(Console::DARK_GREY) << glGetString(GL_RENDERER) << ConsoleColor() << std::endl;
	std::cout << "\tGLSL Version: " << ConsoleColor(Console::DARK_GREY) << glGetString(GL_SHADING_LANGUAGE_VERSION) << ConsoleColor() << std::endl;

	// Print extensions
	std::cout << "\tExtensions: " << ConsoleColor(Console::DARK_GREY);
	int nExtensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &nExtensions);
	for (int i = 0; i < nExtensions; i++) {
		String str = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
		std::cout << str << " ";
	}
	std::cout << ConsoleColor() << std::endl;

	setupDebugCallback();

	SDL_GL_SetSwapInterval(vsync ? 1 : 0);
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

void VideoOpenGL::setupDebugCallback()
{
	if (glDebugMessageCallback) {
		glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
			reinterpret_cast<const VideoOpenGL*>(userParam)->onGLDebugMessage(source, type, id, severity, message);
		}, this);
		glCheckError();
	} else {
		glGetError();
		std::cout << ConsoleColor(Console::YELLOW) << "KHR_DEBUG is not available." << ConsoleColor() << std::endl;
	}
}

void VideoOpenGL::onSuspend()
{
	if (glDebugMessageCallback) {
		glDebugMessageCallback(nullptr, nullptr);
		glCheckError();
	}
}

void VideoOpenGL::onResume()
{
	initGLBindings();
	setupDebugCallback();
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
}

void VideoOpenGL::onGLDebugMessage(unsigned int source, unsigned int type, unsigned int id, unsigned int severity, String message) const
{
	if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_LOW) {
		std::stringstream ss;
#if HAS_EASTL
		ss << "[" << glEnumMap.at(source).second << "] [" << glEnumMap.at(type).second << "] [" << glEnumMap.at(severity).second << "] " << id << ": " << message;
#else
		ss << "[" << glEnumMap.at(source) << "] [" << glEnumMap.at(type) << "] [" << glEnumMap.at(severity) << "] " << id << ": " << message;
#endif
		std::string str = ss.str();

		std::lock_guard<std::mutex> lock(messagesMutex);
		messagesPending.push_back([str] () {
			std::cout << ConsoleColor(Console::YELLOW) << str << ConsoleColor() << std::endl;
		});		
	}
}

std::function<void(int, void*)> VideoOpenGL::getUniformBinding(UniformType type, int n)
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
			throw Exception("Unsupported uniform type: " + String::integerToString(static_cast<int>(type)));
	}
}

std::unique_ptr<Painter> VideoOpenGL::makePainter()
{
	return std::make_unique<PainterOpenGL>();
}

std::unique_ptr<Texture> VideoOpenGL::createTexture(const TextureDescriptor& descriptor)
{
	return std::make_unique<TextureOpenGL>(descriptor);
}

std::unique_ptr<Shader> VideoOpenGL::createShader(String name)
{
	return std::make_unique<ShaderOpenGL>(name);
}

std::unique_ptr<TextureRenderTarget> VideoOpenGL::createRenderTarget()
{
	return std::make_unique<RenderTargetOpenGL>();
}

Vector2i VideoOpenGL::getScreenSize(int n) const
{
	if (n >= SDL_GetNumVideoDisplays()) {
		return Vector2i();
	}
	SDL_DisplayMode info;
	SDL_GetDesktopDisplayMode(n, &info);
	return Vector2i(info.w, info.h);
}

void VideoOpenGL::flip()
{
	SDL_GL_SwapWindow(sdlWindow);

	Vector<std::function<void()>> msgs;
	{
		std::lock_guard<std::mutex> lock(messagesMutex);
		msgs = std::move(messagesPending);
	}
	for (const auto& m: msgs) {
		m();
	}
}

void VideoOpenGL::processEvent(SDL_Event& event)
{
	if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
		Vector2i size = Vector2i(event.window.data1, event.window.data2);
		int x, y;
		SDL_GetWindowPosition(sdlWindow, &x, &y);
		updateWindow(getWindow().withPosition(Vector2i(x, y)).withSize(size));
	}
}

Rect4i VideoOpenGL::getWindowRect() const
{
	int x, y, w, h;
	SDL_GetWindowPosition(sdlWindow, &x, &y);
	SDL_GetWindowSize(sdlWindow, &w, &h);
	return Rect4i(x, y, w, h);
}

Rect4i VideoOpenGL::getDisplayRect(int screen) const
{
	screen = std::max(0, std::min(screen, SDL_GetNumVideoDisplays() - 1));

	SDL_Rect rect;
	SDL_GetDisplayBounds(screen, &rect);
	return Rect4i(rect.x, rect.y, rect.w, rect.h);
}

Vector2i VideoOpenGL::getCenteredWindow(Vector2i size, int screen) const
{
	Rect4i rect = getDisplayRect(screen);
	return rect.getTopLeft() + (rect.getSize() - size) / 2;
}

void VideoOpenGL::startRender()
{
	Debug::trace("VideoOpenGL::startRender");

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
	Debug::trace("VideoOpenGL::finishRender flipping");
	flip();
	Debug::trace("VideoOpenGL::finishRender end");

	glCheckError();
}
