#include "win32_window.h"
#include <winuser.h>

#include "win32_system.h"

using namespace Halley;

LRESULT CALLBACK HalleyWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_GETMINMAXINFO:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	case WM_NCCREATE:
		return true;
	case WM_NCCALCSIZE:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	case WM_CREATE:
		{
			CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
			void* window = createStruct->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	default:
		{
			auto window = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			return window->onMessage(uMsg, wParam, lParam);
		}
	}
}

Win32Window::Win32Window(const WindowDefinition& def, Win32System& system)
	: definition(def)
	, system(system)
{
	auto hInstance = GetModuleHandle(nullptr);

	const static char* windowClassName = "HalleyWindow";
	WNDCLASS wc = {};
	wc.lpfnWndProc = HalleyWindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = windowClassName;
	RegisterClass(&wc);

	const Vector2i pos = def.getPosition().value_or(Vector2i(0, 0));
	const Vector2i size = def.getSize();
	
	DWORD style = WS_OVERLAPPEDWINDOW;
	hwnd = CreateWindowEx(0, windowClassName, definition.getTitle().c_str(), style, pos.x, pos.y, size.x, size.y, nullptr, nullptr, hInstance, this);
	if (!hwnd) {
		const auto error = GetLastError();
		throw Exception("Unable to create window, error: " + toString(error, 16), HalleyExceptions::SystemPlugin);
	}
}

Win32Window::~Win32Window()
{
	destroy();
}

void Win32Window::update(const WindowDefinition& def)
{
	definition = def;
}

void Win32Window::show()
{
	ShowWindow(hwnd, 1);
}

void Win32Window::hide()
{
	ShowWindow(hwnd, 0);
}

void Win32Window::setVsync(bool vsync)
{
	// TODO
}

void Win32Window::swap()
{
	// TODO
}

Rect4i Win32Window::getWindowRect() const
{
	// TODO
	return Rect4i(0, 0, 1920, 1080);
}

const WindowDefinition& Win32Window::getDefinition() const
{
	return definition;
}

void Win32Window::destroy()
{
	if (hwnd) {
		DestroyWindow(hwnd);
	}
	hwnd = nullptr;
	system.onWindowDestroyed(*this);
}

void* Win32Window::getNativeHandle()
{
	return reinterpret_cast<void*>(hwnd);
}

String Win32Window::getNativeHandleType()
{
	return "Win32";
}

LRESULT Win32Window::onMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_DESTROY:
		destroy();
		return 0;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

bool Win32Window::isAlive() const
{
	return hwnd != nullptr;
}
