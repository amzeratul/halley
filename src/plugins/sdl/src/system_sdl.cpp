#include "system_sdl.h"
#include <SDL.h>
#include <fstream>

#include "halley/api/halley_api_internal.h"
#include <halley/support/console.h>
#include <halley/support/exception.h>
#include "sdl_rw_ops.h"
#include "halley/graphics/window.h"
#include "halley/os/os.h"
#include "sdl_window.h"
#include "sdl_gl_context.h"
#include "input_sdl.h"
#include "halley/support/logger.h"
#include "sdl_save.h"
#include "halley/game/game_platform.h"

#ifdef _WIN32
#define WIN32_WIN_AND_MEAN
#endif
#include <SDL_syswm.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

using namespace Halley;

SystemSDL::SystemSDL(std::optional<String> saveCryptKey)
	: saveCryptKey(std::move(saveCryptKey))
{
}

void SystemSDL::init()
{
	// SDL version
	SDL_version compiled;
	SDL_version linked;
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	std::cout << ConsoleColour(Console::GREEN) << "\nInitializing SDL..." << ConsoleColour() << std::endl;
	std::cout << "\tVersion/Compiled: " << ConsoleColour(Console::DARK_GREY) << int(compiled.major) << "." << int(compiled.minor) << "." << int(compiled.patch) << ConsoleColour() << std::endl;
	std::cout << "\tVersion/Linked: " << ConsoleColour(Console::DARK_GREY) << int(linked.major) << "." << int(linked.minor) << "." << int(linked.patch) << ConsoleColour() << std::endl;

	// Initialize SDL
	if (!SDL_WasInit(0)) {
		if (SDL_Init(0) == -1) {
			throw Exception(String("Exception initializing SDL: ") + SDL_GetError(), HalleyExceptions::SystemPlugin);
		}
	}
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
	if (SDL_InitSubSystem(SDL_INIT_TIMER) == -1) {
		throw Exception(String("Exception initializing timer: ") + SDL_GetError(), HalleyExceptions::SystemPlugin);
	}
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) {
		Logger::logWarning("Couldn't initialize SDL Joystick subsystem");
	}
	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == -1) {
		Logger::logWarning("Couldn't initialize SDL GameController subsystem");
	}

	SDL_ShowCursor(SDL_DISABLE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

	// Init clipboard
	clipboard = OS::get().getClipboard();
}

void SystemSDL::deInit()
{
	// Close SDL
	SDL_Quit();
}

void SystemSDL::onResume()
{
	if (SDL_InitSubSystem(SDL_INIT_EVENTS) == -1) {
		throw Exception(String("Exception initializing events: ") + SDL_GetError(), HalleyExceptions::SystemPlugin);
	}
	if (SDL_InitSubSystem(SDL_INIT_TIMER) == -1) {
		throw Exception(String("Exception initializing timer: ") + SDL_GetError(), HalleyExceptions::SystemPlugin);
	}
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1) {
		Logger::logWarning("Couldn't initialize SDL Joystick subsystem");
	}
	if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == -1) {
		Logger::logWarning("Couldn't initialize SDL Joystick subsystem");
	}
}

void SystemSDL::onSuspend()
{
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER);
}

Path SystemSDL::getAssetsPath(const Path& gamePath) const
{
	if constexpr (getPlatform() == GamePlatform::Emscripten) {
		return "assets";
	}

#if defined(HALLEY_MACOSX_BUNDLE)
	return gamePath / ".." / "Resources";
#else
	return gamePath / ".." / "assets";
#endif
}

Path SystemSDL::getUnpackedAssetsPath(const Path& gamePath) const
{
	return gamePath / ".." / "assets_unpacked";
}

bool SystemSDL::generateEvents(VideoAPI* video, InputAPI* input)
{
	auto sdlInput = dynamic_cast<InputSDL*>(input);
	std::array<SDL_Event, 32> events;
	SDL_PumpEvents();
	while (true) {
		const int nEvents = SDL_PeepEvents(events.data(), gsl::narrow_cast<int>(events.size()), SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
		if (nEvents == 0) {
			break;
		}

		for (int i = 0; i < nEvents; ++i) {
			auto& event = events[i];

			switch (event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			case SDL_TEXTINPUT:
			case SDL_TEXTEDITING:
			case SDL_JOYAXISMOTION:
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			case SDL_JOYHATMOTION:
			case SDL_JOYBALLMOTION:
			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEWHEEL:
			case SDL_FINGERUP:
			case SDL_FINGERDOWN:
			case SDL_FINGERMOTION:
			case SDL_CONTROLLERAXISMOTION:
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
			case SDL_CONTROLLERDEVICEADDED:
			case SDL_CONTROLLERDEVICEREMOVED:
			case SDL_CONTROLLERDEVICEREMAPPED:
				{
					if (sdlInput) {
						sdlInput->processEvent(event);
					}
					break;
				}
			case SDL_QUIT:
				{
					std::cout << "SDL_QUIT received." << std::endl;
					return false;
				}
			case SDL_WINDOWEVENT:
				{
					if (video) {
						processVideoEvent(video, event);
					}
					break;
				}
			case SDL_SYSWMEVENT:
				processSystemEvent(event);
				break;
			}
		}
	}
	return true;
}

void SystemSDL::processVideoEvent(VideoAPI* video, const SDL_Event& event)
{
	for (auto& w : windows) {
		if (w->getId() == int(event.window.windowID)) {
			if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
				int x, y;
				SDL_GetWindowPosition(SDL_GetWindowFromID(event.window.windowID), &x, &y);
				w->updateDefinition(w->getDefinition().withPosition(Vector2i(x, y)).withSize(Vector2i(event.window.data1, event.window.data2)));
			} else if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
				w->updateDefinition(w->getDefinition().withState(WindowState::Maximized));
			} else if (event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
				w->updateDefinition(w->getDefinition().withState(WindowState::Minimized));
			} else if (event.window.event == SDL_WINDOWEVENT_RESTORED) {
				w->updateDefinition(w->getDefinition().withState(WindowState::Normal));
			} else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
				w->updateDefinition(w->getDefinition().withFocus(true));
			} else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
				w->updateDefinition(w->getDefinition().withFocus(false));
			}
		}
	}
}

void SystemSDL::processSystemEvent(const SDL_Event& event)
{
#ifdef _WIN32
	const auto& winMsg = event.syswm.msg->msg.win;
	if (winMsg.msg == WM_HOTKEY) {
		const int key = static_cast<int>(winMsg.wParam);
		if (key >= 0 && key < static_cast<int>(globalHotkeyCallbacks.size())) {
			globalHotkeyCallbacks[key]();
		}
	}
#endif
}

#ifdef _WIN32
static int sdlToWin32KeyCode(KeyCode key)
{
	if (key == KeyCode::Keypad0) {
		return VK_NUMPAD0;
	}
	if (key >= KeyCode::Keypad1 && key <= KeyCode::Keypad9) {
		return VK_NUMPAD1 + (int(key) - int(KeyCode::Keypad1));
	}
	if (key >= KeyCode::A && key <= KeyCode::Z) {
		return 'A' + (int(key) - int(KeyCode::A));
	}
	if (key >= KeyCode::Num0 && key <= KeyCode::Num9) {
		return '0' + (int(key) - int(KeyCode::Num0));
	}
	Logger::logWarning("Unknown key mapping on SDL -> Win32");
	return 0;
}

static int sdlToWin32KeyMod(KeyMods mod)
{
	const int modVal = static_cast<int>(mod);
	int value = 0;
	if ((modVal & static_cast<int>(KeyMods::Alt)) != 0) {
		value |= MOD_ALT;
	}
	if ((modVal & static_cast<int>(KeyMods::Ctrl)) != 0) {
		value |= MOD_CONTROL;
	}
	if ((modVal & static_cast<int>(KeyMods::Shift)) != 0) {
		value |= MOD_SHIFT;
	}
	if ((modVal & static_cast<int>(KeyMods::Mod)) != 0) {
		value |= MOD_WIN;
	}
	return value;
}
#endif

void SystemSDL::registerGlobalHotkey(KeyCode key, KeyMods mods, std::function<void()> callback)
{
#ifdef _WIN32
	HWND hWnd = windows.empty() ? GetActiveWindow() : reinterpret_cast<HWND>(windows[0]->getNativeHandle());
	bool success = RegisterHotKey(hWnd, static_cast<int>(globalHotkeyCallbacks.size()), sdlToWin32KeyMod(mods), sdlToWin32KeyCode(key));
#endif
	globalHotkeyCallbacks.push_back(std::move(callback));
}

std::unique_ptr<ResourceDataReader> SystemSDL::getDataReader(String path, int64_t start, int64_t end)
{
	return SDLRWOps::fromPath(path, start, end);
}

std::shared_ptr<Halley::Window> SystemSDL::createWindow(const WindowDefinition& windowDef)
{
	initVideo();

	const auto glVersion = windowDef.getWindowGLVersion();

	// Set flags and GL attributes
	auto windowType = windowDef.getWindowType();
	int flags = SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_ALLOW_HIGHDPI;
#if !defined(WITH_VULKAN)
	if (glVersion) {
		flags |= SDL_WINDOW_OPENGL;
	}
#endif
	if (windowType == WindowType::BorderlessWindow) {
		flags |= SDL_WINDOW_BORDERLESS;
	}
	else if (windowType == WindowType::ResizableWindow) {
		flags |= SDL_WINDOW_RESIZABLE;
	}
	else if (windowType == WindowType::Fullscreen) {
		if (getPlatform() == GamePlatform::MacOS) {
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		} else {
			flags |= SDL_WINDOW_FULLSCREEN;
		}
	}
	if (windowDef.isShowOnCreation()) {
		flags |= SDL_WINDOW_SHOWN;
	} else {
		flags |= SDL_WINDOW_HIDDEN;
	}

#ifdef WITH_VULKAN
	flags |= SDL_WINDOW_VULKAN;
#else
	// Context options
	if (glVersion) {
#if defined(WITH_OPENGL_ES2)
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#elif defined(WITH_OPENGL_ES3)
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, glVersion->versionMajor);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, glVersion->versionMinor);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
	}
#endif

	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	// Disable High-DPI rendering (e.g. for Retina screens on Mac OS). Halley
	// currently has a number of bugs when attempting to render at High-DPI.
	SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");

	// Window position
	Vector2i windowSize = windowDef.getSize();
	Vector2i winPos = windowDef.getPosition().value_or(getCenteredWindow(windowSize, 0));

	// Create window
	auto sdlWindow = SDL_CreateWindow(windowDef.getTitle().c_str(), winPos.x, winPos.y, windowSize.x, windowSize.y, flags);
	if (!sdlWindow) {
		throw Exception(String("Error creating SDL window: ") + SDL_GetError(), HalleyExceptions::SystemPlugin);
	}

	// Show window
	auto window = std::make_shared<SDLWindow>(sdlWindow);
	if (windowDef.isShowOnCreation()) {
		window->show();
	}
	window->update(windowDef);
	windows.push_back(window);
	
	return window;
}

void SystemSDL::destroyWindow(std::shared_ptr<Window> window)
{
	for (size_t i = 0; i < windows.size(); ++i) {
		if (windows[i] == window) {
			windows[i]->destroy();
			windows.erase(windows.begin() + i);
			break;
		}
	}
	if (windows.empty()) {
		deInitVideo();
	}
}

Vector2i SystemSDL::getScreenSize(int n) const
{
	initVideo();
	SDL_DisplayMode info;
	if (SDL_GetDesktopDisplayMode(n, &info) == 0) {
		return Vector2i(info.w, info.h);
	} else {
		return Vector2i();
	}
}

Rect4i SystemSDL::getDisplayRect(int screen) const
{
	screen = std::max(0, std::min(screen, SDL_GetNumVideoDisplays() - 1));

	SDL_Rect rect;
	SDL_GetDisplayBounds(screen, &rect);
	return Rect4i(rect.x, rect.y, rect.w, rect.h);
}

Vector2i SystemSDL::getCenteredWindow(Vector2i size, int screen) const
{
	Rect4i rect = getDisplayRect(screen);
	return rect.getTopLeft() + (rect.getSize() - size) / 2;
}

std::unique_ptr<GLContext> SystemSDL::createGLContext()
{
	return std::make_unique<SDLGLContext>(windows[0]->getSDLWindow());
}

std::shared_ptr<Halley::SDLWindow> SystemSDL::getWindow(int index)
{
	if (index >= windows.size())
		return nullptr;

	return windows[index];
}

void SystemSDL::showCursor(bool show)
{
	SDL_ShowCursor(show ? 1 : 0);
}

std::shared_ptr<ISaveData> SystemSDL::getStorageContainer(SaveDataType type, const String& containerName)
{
	Path dir = saveDir[type];
	if (!containerName.isEmpty()) {
		dir = dir / containerName / ".";
	}

	return std::make_shared<SDLSaveData>(type, dir, saveCryptKey);
}

void SystemSDL::setEnvironment(Environment* env)
{
	for (int i = 0; i <= int(SaveDataType::Cache); ++i) {
		SaveDataType type = SaveDataType(i);
		auto dir = env->getDataPath() / toString(type) / ".";
		saveDir[type] = dir;
	}
}

bool SystemSDL::mustOwnMainLoop() const
{
	return getPlatform() == GamePlatform::Emscripten;
}

void SystemSDL::setGameLoopHandler(std::unique_ptr<ISystemMainLoopHandler> handler)
{
	this->mainLoopHandler = std::move(handler);
#ifdef __EMSCRIPTEN__
	emscripten_request_animation_frame_loop([](double t, void* ptr) -> EM_BOOL {
		auto* system = static_cast<SystemSDL*>(ptr);
		const bool run = system->mainLoopHandler->run();
		if (!run) {
			system->mainLoopHandler = {};
		}
		return run ? EM_TRUE : EM_FALSE;
	}, this);
#endif
}

bool SystemSDL::canExit()
{
	return getPlatform() != GamePlatform::Emscripten;
}

std::shared_ptr<IClipboard> SystemSDL::getClipboard() const
{
	return clipboard;
}


#if defined(_WIN32) && !defined(WINDOWS_STORE)
#pragma warning(default: 6320 6322)
#include <Windows.h>
const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
#pragma warning(disable: 6320 6322)

void SetThreadName( DWORD dwThreadID, const char* name)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR*>(&info) );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}
#endif

void SystemSDL::setThreadName(const String& name)
{
#if defined(_WIN32) && !defined(WINDOWS_STORE) && defined(_DEBUG)
	if (name != "main") {
		SetThreadName(static_cast<DWORD>(-1), name.c_str());
	}
#endif
}

void SystemSDL::setThreadPriority(ThreadPriority priority)
{
#if defined(_WIN32) 
	int priorityValue = 0;
	switch (priority) {
	case ThreadPriority::VeryLow:
		priorityValue = THREAD_PRIORITY_LOWEST;
		break;
	case ThreadPriority::Low:
		priorityValue = THREAD_PRIORITY_BELOW_NORMAL;
		break;
	case ThreadPriority::Normal:
		priorityValue = THREAD_PRIORITY_NORMAL;
		break;
	case ThreadPriority::High:
		priorityValue = THREAD_PRIORITY_ABOVE_NORMAL;
		break;
	case ThreadPriority::VeryHigh:
		priorityValue = THREAD_PRIORITY_HIGHEST;
		break;
	}
	const HANDLE thread = GetCurrentThread();
	SetThreadPriority(thread, priorityValue);
#endif
}

void SystemSDL::printDebugInfo() const
{
	std::cout << std::endl << ConsoleColour(Console::GREEN) << "Initializing Video Display...\n" << ConsoleColour();
	std::cout << "Drivers available:\n";
	for (int i = 0; i < SDL_GetNumVideoDrivers(); i++) {
		std::cout << "\t" << i << ": " << SDL_GetVideoDriver(i) << "\n";
	}
	std::cout << "Video driver: " << ConsoleColour(Console::DARK_GREY) << SDL_GetCurrentVideoDriver() << ConsoleColour() << std::endl;
}

void SystemSDL::initVideo() const
{
	if (!videoInit) {
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1) {
			throw Exception(String("Exception initializing video: ") + SDL_GetError(), HalleyExceptions::SystemPlugin);
		}
		SDL_VideoInit(nullptr);
		printDebugInfo();
		videoInit = true;
	}
}

void SystemSDL::deInitVideo()
{
	if (videoInit) {
		SDL_VideoQuit();
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		videoInit = false;
	}
}