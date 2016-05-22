#include <SDL.h>
#include <GL/glew.h>
#include "halley_gl.h"
#include "video_opengl.h"
#include "painter_opengl.h"
#include "texture_opengl.h"
#include "shader_opengl.h"
#include "render_target_opengl.h"
using namespace Halley;

#ifdef _MSC_VER
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "opengl32.lib")
#endif

void VideoOpenGL::init()
{
	
}

void VideoOpenGL::deInit()
{
	if (running) {
		running = false;
		//loaderThread.join();
	}

	SDL_GL_MakeCurrent(window, nullptr);
	SDL_GL_DeleteContext(context);
	context = nullptr;
	SDL_DestroyWindow(window);
	window = nullptr;
	SDL_VideoQuit();

	std::cout << "Video terminated." << std::endl;
}


///////////////
// Constructor
VideoOpenGL::VideoOpenGL()
	: windowType(WindowType::None)
	, initialized(false)
	, running(false)
	, border(0)
{
}


/////////////
// Set video
void VideoOpenGL::setVideo(WindowType _windowType, const Vector2i _fullscreenSize, const Vector2i _windowedSize, const Vector2f _virtualSize, bool vsync, int screen)
{
	bool wasInit = initialized;

	// Initialize video mode
	if (!wasInit) {
		SDL_VideoInit(nullptr);
	}

	// Window size
	Vector2i windowSize = _windowType == WindowType::Fullscreen ? _fullscreenSize : _windowedSize;

#ifdef __ANDROID__
	// Android-specific overrides, since it should always be fullscreen and on the actual window size
	_fullscreen = true;
	windowSize = VideoOpenGL::getScreenSize();
#endif

	// Debug info
	//char buffer[1024];
	std::cout << std::endl << ConsoleColor(Console::GREEN) << "Initializing OpenGL...\n" << ConsoleColor();
	std::cout << "Drivers available:\n";
	for (int i = 0; i < SDL_GetNumVideoDrivers(); i++) {
		std::cout << "\t" << i << ": " << SDL_GetVideoDriver(i) << "\n";
	}
	std::cout << "Video driver: " << ConsoleColor(Console::DARK_GREY) << SDL_GetCurrentVideoDriver() << ConsoleColor() << std::endl;
	std::cout << "Window size: " << ConsoleColor(Console::DARK_GREY) << windowSize.x << "x" << windowSize.y << ConsoleColor() << std::endl;

	// Store values
	fullscreenSize = _fullscreenSize;
	windowedSize = _windowedSize;
	windowType = _windowType;
	virtualSize = _virtualSize;
	setWindowSize(windowSize);

	// Window name
	//String name = game.getName();
	//if (game.isDevBuild()) name += " [DEV BUILD]";
	String name = "Halley game";

	if (!wasInit) {
		// Set flags and GL attributes
		int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS;
		if (_windowType == WindowType::BorderlessWindow) {
			flags |= SDL_WINDOW_BORDERLESS;
		} else if (_windowType == WindowType::ResizableWindow) {
			flags |= SDL_WINDOW_RESIZABLE;
		}
		//if (_fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
		
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
#ifndef _DEBUG
		//SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#endif
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

		// Window position
		Vector2i winPos(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
		if (screen < SDL_GetNumVideoDisplays()) {
			SDL_Rect rect;
			SDL_GetDisplayBounds(screen, &rect);
			winPos.x = rect.x + (rect.w - windowSize.x) / 2;
			winPos.y = rect.y + (rect.h - windowSize.y) / 2;
		}

		// Create window
		window = SDL_CreateWindow(name.c_str(), winPos.x, winPos.y, windowSize.x, windowSize.y, flags);
		if (!window)
			throw Exception(String("Error creating SDL window: ") + SDL_GetError());
#ifndef __ANDROID__
		SDL_SetWindowFullscreen(window, _windowType == WindowType::Fullscreen ? SDL_TRUE : SDL_FALSE);
#endif

		// Create OpenGL context
		SDL_GLContext context = SDL_GL_CreateContext(window);
		if (!context)
			throw Exception(String("Error creating OpenGL context: ") + SDL_GetError());
		if (SDL_GL_MakeCurrent(window, context) < 0)
			throw Exception(String("Error setting OpenGL context: ") + SDL_GetError());
		context = context;

		// VSync
		SDL_GL_SetSwapInterval(vsync ? 1 : 0);

		// Start loader thread
		if (!running) {
#ifdef ____WIN32__
			vid->loaderThread = TextureLoadQueue::startLoaderThread(window, &vid->running);
			vid->running = true;
#endif
		}

		// Initialize GLEW
#ifdef WITH_OPENGL
		glewExperimental = true;
		GLenum err = glewInit();
		if (err != GLEW_OK) {
			throw Exception(String("Error initializing GLEW: ") + reinterpret_cast<const char*>(glewGetErrorString(err)));
		}
		glGetError(); // discard error
		glCheckError();
#endif

		// Print OpenGL data
		std::cout << "OpenGL initialized." << std::endl;
		std::cout << "\tVersion: " << ConsoleColor(Console::DARK_GREY) << glGetString(GL_VERSION) << ConsoleColor() << std::endl;
		std::cout << "\tVendor: " << ConsoleColor(Console::DARK_GREY) << glGetString(GL_VENDOR) << ConsoleColor() << std::endl;
		std::cout << "\tRenderer: " << ConsoleColor(Console::DARK_GREY) << glGetString(GL_RENDERER) << ConsoleColor() << std::endl;
		std::cout << "\tGLSL Version: " << ConsoleColor(Console::DARK_GREY) << glGetString(GL_SHADING_LANGUAGE_VERSION) << ConsoleColor() << std::endl;

		// Render-to-texture support
		bool fboSupport = true;//TextureRenderTargetFBO::isSupported();
		std::cout << "\tFramebuffer Objects: " << ConsoleColor(Console::DARK_GREY) << (fboSupport ? "enabled" : "disabled") << ConsoleColor() << std::endl;
#if defined(WITH_OPENGL)
		int n;
		glGetIntegerv(GL_AUX_BUFFERS, &n);
		//if (n > 0) TextureRenderTargetCopy::hasAuxBuffer = true;
#endif

		// Print extensions
		std::cout << "\tExtensions: " << ConsoleColor(Console::DARK_GREY);
		int nExtensions;
		glGetIntegerv(GL_NUM_EXTENSIONS, &nExtensions);
		for (int i = 0; i < n; i++) {
			String str = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
			std::cout << str << " ";
		}
		std::cout << ConsoleColor() << std::endl;
	} else {
		// Re-initializing

		// Update window
#ifndef __ANDROID__
		if (_windowType != WindowType::Fullscreen) SDL_SetWindowFullscreen(window, SDL_FALSE);
		SDL_SetWindowSize(window, windowSize.x, windowSize.y);
		if (_windowType == WindowType::Fullscreen) SDL_SetWindowFullscreen(window, SDL_TRUE);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
#endif
	}

	// Clear buffer
	glCheckError();
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glCheckError();

	// Swap buffers
	flip();

	// Show window
	SDL_ShowWindow(window);
	initialized = true;
	std::cout << ConsoleColor(Console::GREEN) << "Video init done.\n" << ConsoleColor() << std::endl;
}

void VideoOpenGL::setWindowSize(Vector2i winSize)
{
	windowSize = winSize;
	updateWindowDimensions();
}

void VideoOpenGL::setVirtualSize(Vector2f vs)
{
	virtualSize = vs;
	updateWindowDimensions();
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

void VideoOpenGL::updateWindowDimensions()
{
	border = 0;
	if (virtualSize.x == 0 || virtualSize.y == 0) {
		virtualSize = p2 = Vector2f(windowSize);
		scale = 1;
	} else {
		float wAR = (float) windowSize.x / (float) windowSize.y;
		float vAR = virtualSize.x / virtualSize.y;
		p1 = Vector2f();
		p2 = virtualSize;
		if (wAR > vAR) {
			// Letterbox on left/right
			scale = windowSize.y / virtualSize.y;
			border = (virtualSize.y * wAR - virtualSize.x) * 0.5f * scale;
			p2 *= scale;
			p1.x += border;
			p2.x += border;
		} else {
			// Letterbox on top/bottom
			//float border = windowSize.y - windowSize.x / vAR;
			scale = windowSize.x / virtualSize.x;
			border = (virtualSize.x / wAR - virtualSize.y) * 0.5f * scale;
			p2 *= scale;
			p1.y += border;
			p2.y += border;
		}
	}
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
	SDL_GL_SwapWindow(window);
}

void VideoOpenGL::setFullscreen(bool fs)
{
	if (fs != (windowType == WindowType::Fullscreen)) {
		setVideo(fs ? WindowType::Fullscreen : WindowType::Window, fullscreenSize, windowedSize, virtualSize);
	}
}

void VideoOpenGL::toggleFullscreen()
{
	setFullscreen(!isFullscreen());
}

void VideoOpenGL::processEvent(SDL_Event& event)
{
	if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
		Vector2i size = Vector2i(event.window.data1, event.window.data2);
		setWindowSize(size);
	}
}

Rect4i VideoOpenGL::getWindowRect() const
{
	int x, y, w, h;
	SDL_GetWindowPosition(window, &x, &y);
	SDL_GetWindowSize(window, &w, &h);
	return Rect4i(x, y, w, h);
}

Rect4i VideoOpenGL::getDisplayRect() const
{
	SDL_Rect rect;
	SDL_GetDisplayBounds(0, &rect);
	return Rect4i(rect.x, rect.y, rect.w, rect.h);
}


/*
static void drawBox(spPainter painter, float x, float y, float w, float h)
{
	GLUtils glUtils;
	glUtils.setNumberOfTextureUnits(1);
	glUtils.setTextureUnit(0);
	glUtils.bindTexture(0);
	glUtils.setBlendType(BlendType::Opaque);
	float vs[] = { x, y, x + w + 1, y, x + w + 1, y + h + 1, x, y + h + 1 };
	painter->drawQuad(Shader::getDefault(), vs);
}
*/

static void drawLetterbox() {
	// TODO
	/*
	Debug::trace("Game::RenderScreen drawing letterboxes");
	Camera::bindScreen();
	Vector2f p = Video::getOrigin();

	// Check if there's any need for it, i.e. window doesn't match game AR
	if (p.y > 0 || p.x > 0) {
		GLUtils glUtils;
		Vector2f s = Video::getVirtualSize();
		Rect4i oldView = glUtils.getViewPort();
		Rect4i view = Rect4i(0, 0, Video::getWindowSize().x, Video::getWindowSize().y);

		// Setting the viewport is necessary to draw outside game bounds, will work on certain drivers even with this off, so be careful
		glUtils.setViewPort(view, false);

		float border = Video::getBorder() / Video::getScale();

		if (p.y > 0) {
			// Top and bottom
			border *= float(oldView.getHeight()) / view.getHeight();
			drawBox(painter, 0, 0, s.x, border);
			drawBox(painter, 0, s.y + 1 - border, s.x, border);
		}
		if (p.x > 0) {
			// Left and right
			border *= float(oldView.getWidth()) / view.getWidth();
			drawBox(painter, 0, 0, border, s.y);
			drawBox(painter, s.x + 1 - border, 0, border, s.y);
		}

		glUtils.setViewPort(oldView);
	}
	Camera::resetBind();
	*/
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
	drawLetterbox();

	Debug::trace("VideoOpenGL::finishRender flipping");
	flip();
	Debug::trace("VideoOpenGL::finishRender end");

	glCheckError();
}
