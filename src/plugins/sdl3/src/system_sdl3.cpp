#include "system_sdl3.h"

#include "halley/api/halley_api_internal.h"
#include <halley/support/console.h>
#include <halley/support/exception.h>
#include "sdl3_rw_ops.h"
#include "halley/graphics/window.h"
#include "halley/os/os.h"
#include "sdl3_window.h"
#include "sdl3_gl_context.h"
#include "input_sdl3.h"
#include "halley/support/logger.h"
#include "../../sdl/src/sdl_save.h"
#include "halley/game/game_platform.h"

#ifdef _WIN32
#define WIN32_WIN_AND_MEAN
#include <Windows.h>
#endif

#ifdef WITH_GDK
#include <appnotify.h>
extern void SDL_GDKSuspendComplete_Proxy();
#endif

using namespace Halley;

#ifdef _WIN32
static bool sdlWindowsMessageHook(void* userData, MSG* msg)
{
    ((SystemSDL3*) userData)->processSystemEvent(msg);
    return true;
}
#endif

SystemSDL3::SystemSDL3(std::optional<String> saveCryptKey)
	: saveCryptKey(std::move(saveCryptKey))
{
}

void SystemSDL3::init()
{
	// SDL version
	int compiled = SDL_VERSION;
	int linked = SDL_GetVersion();

	std::cout << ConsoleColour(Console::GREEN) << "\nInitializing SDL..." << ConsoleColour() << std::endl;
	std::cout << "\tVersion/Compiled: " << ConsoleColour(Console::DARK_GREY) << SDL_VERSIONNUM_MAJOR(compiled) << "." << SDL_VERSIONNUM_MINOR(compiled) << "." << SDL_VERSIONNUM_MICRO(compiled) << ConsoleColour() << std::endl;
	std::cout << "\tVersion/Linked: " << ConsoleColour(Console::DARK_GREY) << SDL_VERSIONNUM_MAJOR(linked) << "." << SDL_VERSIONNUM_MINOR(linked) << "." << SDL_VERSIONNUM_MICRO(linked) << ConsoleColour() << std::endl;

	// Initialize SDL
	if (!SDL_WasInit(0)) {
		if (!SDL_Init(0)) {
			throw Exception(String("Exception initializing SDL: ") + SDL_GetError(), HalleyExceptions::SystemPlugin);
		}
	}
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

#if defined(_GAMING_XBOX_XBOXONE) || defined(_GAMING_XBOX_SCARLETT)
	// Xbox: need to enable GameInput explicitly
	SDL_SetHint(SDL_HINT_JOYSTICK_GAMEINPUT, "1");
#endif

	if (!SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
		Logger::logWarning("Couldn't initialize SDL Joystick subsystem");
	}
	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
		Logger::logWarning("Couldn't initialize SDL GameController subsystem");
	}

	SDL_HideCursor();

#ifdef _WIN32
    SDL_SetWindowsMessageHook(sdlWindowsMessageHook, this);
#endif

	// Init clipboard
	clipboard = OS::get().getClipboard();
}

void SystemSDL3::deInit()
{
	// Close SDL
	SDL_Quit();
}

void SystemSDL3::onResume()
{
	if (!SDL_InitSubSystem(SDL_INIT_EVENTS)) {
		throw Exception(String("Exception initializing events: ") + SDL_GetError(), HalleyExceptions::SystemPlugin);
	}
	if (!SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
		Logger::logWarning("Couldn't initialize SDL Joystick subsystem");
	}
	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
		Logger::logWarning("Couldn't initialize SDL Joystick subsystem");
	}
}

void SystemSDL3::onSuspend()
{
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD);
}

Path SystemSDL3::getAssetsPath(const Path& gamePath) const
{
#if defined(HALLEY_MACOSX_BUNDLE)
	return gamePath / ".." / "Resources";
#elif defined(_GAMING_XBOX)
	return gamePath / "assets";
#elif defined(_GAMING_DESKTOP) && defined(DEV_BUILD)
	return gamePath / "../.." / "assets";
#else
	return gamePath / ".." / "assets";
#endif
}

Path SystemSDL3::getUnpackedAssetsPath(const Path& gamePath) const
{
#if defined(_GAMING_XBOX)
	return gamePath / "assets_unpacked";
#elif defined(_GAMING_DESKTOP) && defined(DEV_BUILD)
	return gamePath / "../.." / "assets_unpacked";
#else
	return gamePath / ".." / "assets_unpacked";
#endif
}

bool SystemSDL3::generateEvents(VideoAPI* video, InputAPI* input)
{
	auto sdlInput = dynamic_cast<InputSDL3*>(input);
	std::array<SDL_Event, 32> events = {};
	SDL_PumpEvents();
	while (true) {
		const int nEvents = SDL_PeepEvents(events.data(), gsl::narrow_cast<int>(events.size()), SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
		if (nEvents == 0) {
			break;
		}

		for (int i = 0; i < nEvents; ++i) {
			auto& event = events[i];

            if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) {
                if (video) {
                    processVideoEvent(video, event);
                }
            } else {
                switch (event.type) {
                    case SDL_EVENT_KEY_DOWN:
                    case SDL_EVENT_KEY_UP:
                    case SDL_EVENT_TEXT_INPUT:
                    case SDL_EVENT_TEXT_EDITING:
                    case SDL_EVENT_JOYSTICK_AXIS_MOTION:
                    case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
                    case SDL_EVENT_JOYSTICK_BUTTON_UP:
                    case SDL_EVENT_JOYSTICK_HAT_MOTION:
                    case SDL_EVENT_JOYSTICK_BALL_MOTION:
                    case SDL_EVENT_MOUSE_MOTION:
                    case SDL_EVENT_MOUSE_BUTTON_UP:
                    case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    case SDL_EVENT_MOUSE_WHEEL:
                    case SDL_EVENT_FINGER_UP:
                    case SDL_EVENT_FINGER_DOWN:
                    case SDL_EVENT_FINGER_MOTION:
                    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                    case SDL_EVENT_GAMEPAD_BUTTON_UP:
                    case SDL_EVENT_GAMEPAD_ADDED:
                    case SDL_EVENT_GAMEPAD_REMOVED:
                    case SDL_EVENT_GAMEPAD_REMAPPED: {
                        if (sdlInput) {
                            sdlInput->processEvent(event);
                        }
                        break;
                    }
                    case SDL_EVENT_QUIT: {
                        std::cout << "SDL_QUIT received." << std::endl;
                        return false;
                    }
                }
            }
		}
	}
	return true;
}

void SystemSDL3::processVideoEvent(VideoAPI* video, const SDL_Event& event)
{
	for (auto& w : windows) {
		if (w->getId() == int(event.window.windowID)) {
			if (event.window.type == SDL_EVENT_WINDOW_RESIZED) {
				int x, y;
				SDL_GetWindowPosition(SDL_GetWindowFromID(event.window.windowID), &x, &y);
				w->updateDefinition(w->getDefinition().withPosition(Vector2i(x, y)).withSize(Vector2i(event.window.data1, event.window.data2)));
			} else if (event.window.type == SDL_EVENT_WINDOW_MAXIMIZED) {
				w->updateDefinition(w->getDefinition().withState(WindowState::Maximized));
			} else if (event.window.type == SDL_EVENT_WINDOW_MINIMIZED) {
				w->updateDefinition(w->getDefinition().withState(WindowState::Minimized));
			} else if (event.window.type == SDL_EVENT_WINDOW_RESTORED) {
				w->updateDefinition(w->getDefinition().withState(WindowState::Normal));
			} else if (event.window.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
				w->updateDefinition(w->getDefinition().withFocus(true));
			} else if (event.window.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
				w->updateDefinition(w->getDefinition().withFocus(false));
			}
		}
	}
}

void SystemSDL3::processSystemEvent(const void* msg)
{
#ifdef _WIN32
	const auto winMsg = (MSG*)msg;
	if (winMsg->message == WM_HOTKEY) {
		const int key = static_cast<int>(winMsg->wParam);
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

void SystemSDL3::registerGlobalHotkey(KeyCode key, KeyMods mods, std::function<void()> callback)
{
#if defined(_WIN32) && !defined(WITH_GDK)
	HWND hWnd = windows.empty() ? GetActiveWindow() : reinterpret_cast<HWND>(windows[0]->getNativeHandle());
	bool success = RegisterHotKey(hWnd, static_cast<int>(globalHotkeyCallbacks.size()), sdlToWin32KeyMod(mods), sdlToWin32KeyCode(key));
#endif
	globalHotkeyCallbacks.push_back(std::move(callback));
}

std::unique_ptr<ResourceDataReader> SystemSDL3::getDataReader(String path, int64_t start, int64_t end)
{
	return SDL3RWOps::fromPath(path, start, end);
}

std::shared_ptr<Halley::Window> SystemSDL3::createWindow(const WindowDefinition& windowDef)
{
	initVideo();

	// Set flags and GL attributes
	auto windowType = windowDef.getWindowType();
	int flags = SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	if (windowType == WindowType::BorderlessWindow) {
		flags |= SDL_WINDOW_BORDERLESS;
	}
	else if (windowType == WindowType::ResizableWindow) {
		flags |= SDL_WINDOW_RESIZABLE;
	}
	else if (windowType == WindowType::Fullscreen) {
		flags |= SDL_WINDOW_FULLSCREEN;
	}
	if (!windowDef.isShowOnCreation()) {
		flags |= SDL_WINDOW_HIDDEN;
	}

#if defined(WITH_OPENGL)
	const auto glVersion = windowDef.getWindowGLVersion();

	if (glVersion) {
		flags |= SDL_WINDOW_OPENGL;
	}

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
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	// Window position
	Vector2i windowSize = windowDef.getSize();
	Vector2i winPos = windowDef.getPosition().value_or(getCenteredWindow(windowSize, 0));

	// Create window
	auto sdlWindow = SDL_CreateWindow(windowDef.getTitle().c_str(), windowSize.x, windowSize.y, flags);
	if (!sdlWindow) {
		throw Exception(String("Error creating SDL window: ") + SDL_GetError(), HalleyExceptions::SystemPlugin);
	}

    SDL_SetWindowPosition(sdlWindow, winPos.x, winPos.y);

	// Show window
	auto window = std::make_shared<SDL3Window>(sdlWindow);
	if (windowDef.isShowOnCreation()) {
		window->show();
	}
	window->update(windowDef);
	windows.push_back(window);

#ifdef WITH_GDK
	PAPPSTATE_REGISTRATION plm = {};
	if (RegisterAppStateChangeNotification([](BOOLEAN quiesced, PVOID context)
	{
		SystemSDL3* self = static_cast<SystemSDL3*>(context);

		Concurrent::execute(Executors::getMainUpdateThread(), [self, quiesced]
		{
			self->onAppStateChangeNotification(quiesced);
		}).wait();

		if (quiesced) {
			SDL_GDKSuspendComplete_Proxy();
		}
	}, this, &plm)) {
		Logger::logWarning("Couldn't register PLM state change notification");
	}
	plmHandle = plm;
#endif

	return window;
}

void SystemSDL3::destroyWindow(std::shared_ptr<Window> window)
{
#ifdef WITH_GDK
	UnregisterAppStateChangeNotification(static_cast<PAPPSTATE_REGISTRATION>(plmHandle));
#endif

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

Vector2i SystemSDL3::getScreenSize(int n) const
{
	initVideo();

    int screen = std::clamp<int>(n, 0, (int) displays.size() - 1);
	const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(displays[screen]);

    if (mode != nullptr) {
        return {mode->w, mode->h};
	} else {
		return {};
	}
}

Rect4i SystemSDL3::getDisplayRect(int screen) const
{
    screen = std::clamp(screen, 0, (int) displays.size() - 1);

	SDL_Rect rect;
	SDL_GetDisplayBounds(screen, &rect);

	return {rect.x, rect.y, rect.w, rect.h};
}

Vector2i SystemSDL3::getCenteredWindow(Vector2i size, int screen) const
{
	Rect4i rect = getDisplayRect(screen);
	return rect.getTopLeft() + (rect.getSize() - size) / 2;
}

std::unique_ptr<GLContext> SystemSDL3::createGLContext()
{
	return std::make_unique<SDL3GLContext>(windows[0]->getSDLWindow());
}

std::shared_ptr<SDL3Window> SystemSDL3::getWindow(int index)
{
	if (index >= windows.size())
		return nullptr;

	return windows[index];
}

void SystemSDL3::showCursor(bool show)
{
    if (show) {
        SDL_ShowCursor();
    } else {
        SDL_HideCursor();
    }
}

std::shared_ptr<ISaveData> SystemSDL3::getStorageContainer(SaveDataType type, const String& containerName)
{
	Path dir = saveDir[type];
	if (!containerName.isEmpty()) {
		dir = dir / containerName / ".";
	}

	return std::make_shared<SDLSaveData>(type, dir, saveCryptKey);
}

void SystemSDL3::setEnvironment(Environment* env)
{
	for (int i = 0; i <= int(SaveDataType::Cache); ++i) {
		SaveDataType type = SaveDataType(i);
		auto dir = env->getDataPath() / toString(type) / ".";
		saveDir[type] = dir;
	}
}

bool SystemSDL3::mustOwnMainLoop() const
{
	return false;
}

void SystemSDL3::setGameLoopHandler(std::unique_ptr<ISystemMainLoopHandler> handler)
{
	this->mainLoopHandler = std::move(handler);
}

bool SystemSDL3::canExit()
{
#if defined(_GAMING_XBOX)
	return false;
#else
	return true;
#endif
}

void SystemSDL3::setOnSuspendCallback(SystemOnSuspendCallback callback)
{
	suspendCallback = callback;
}

void SystemSDL3::setOnResumeCallback(SystemOnResumeCallback callback)
{
	resumeCallback = callback;
}

#ifdef WITH_GDK
void SystemSDL3::onAppStateChangeNotification(bool suspended) const
{
	if (suspended && suspendCallback) {
		suspendCallback();
	} else if (!suspended && resumeCallback) {
		resumeCallback();
	}
}
#endif

std::shared_ptr<IClipboard> SystemSDL3::getClipboard() const
{
	return clipboard;
}

void SystemSDL3::setThreadName(const String& name)
{
#if defined(_WIN32) && !defined(WITH_GDK)
	if (name != "main") {
        SetThreadDescription(GetCurrentThread(), name.getUTF16().c_str());
	}
#endif
}

void SystemSDL3::setThreadPriority(ThreadPriority priority)
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
	HANDLE thread = GetCurrentThread();
	SetThreadPriority(thread, priorityValue);
#endif
}

void SystemSDL3::printDebugInfo() const
{
	std::cout << std::endl << ConsoleColour(Console::GREEN) << "Initializing Video Display...\n" << ConsoleColour();
	std::cout << "Drivers available:\n";
	for (int i = 0; i < SDL_GetNumVideoDrivers(); i++) {
		std::cout << "\t" << i << ": " << SDL_GetVideoDriver(i) << "\n";
	}
	std::cout << "Video driver: " << ConsoleColour(Console::DARK_GREY) << SDL_GetCurrentVideoDriver() << ConsoleColour() << std::endl;
}

void SystemSDL3::initVideo() const
{
	if (!videoInit) {
		if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
			throw Exception(String("Exception initializing video: ") + SDL_GetError(), HalleyExceptions::SystemPlugin);
		}
		printDebugInfo();
		videoInit = true;

        int numDisplays;
        auto sdlDisplays = SDL_GetDisplays(&numDisplays);

        if (sdlDisplays != nullptr) {
            displays.resize(numDisplays);
            memcpy(displays.data(), sdlDisplays, numDisplays * sizeof(SDL_DisplayID));
            SDL_free(sdlDisplays);
        } else {
            displays.emplace_back(SDL_GetPrimaryDisplay());
        }
	}
}

void SystemSDL3::deInitVideo()
{
	if (videoInit) {
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		videoInit = false;
	}
}