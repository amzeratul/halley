#include "win32_window.h"
#include <winuser.h>

#include "win32_system.h"

using namespace Halley;

LRESULT CALLBACK HalleyWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_GETMINMAXINFO:
		{
			MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
			return 0;
		}
	case WM_NCCREATE:
		return true;
	case WM_NCCALCSIZE:
		{
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
			//RECT* rect = reinterpret_cast<RECT*>(lParam);
			//return 0;
		}
	case WM_CREATE:
		{
			CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
			void* window = createStruct->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
			return 0;
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

	const Vector2i pos = def.getPosition().value_or(Vector2i(CW_USEDEFAULT, CW_USEDEFAULT));
	const Vector2i size = def.getSize();

	DWORD style = WS_OVERLAPPEDWINDOW;
	DWORD exStyle = 0;

	RECT rect = {};
	rect.left = 0;
	rect.top = 0;
	rect.right = size.x;
	rect.bottom = size.y;
	AdjustWindowRectEx(&rect, style, false, exStyle);

	icon = LoadIcon(hInstance, "IDI_MAIN_ICON");
	
	hwnd = CreateWindowEx(exStyle, windowClassName, "HalleyWindow", style, pos.x, pos.y, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, hInstance, this);
	if (!hwnd) {
		const auto error = GetLastError();
		throw Exception("Unable to create window, error: " + toString(error, 16), HalleyExceptions::SystemPlugin);
	}

	SetWindowText(hwnd, definition.getTitle().c_str());
	SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(icon));
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
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
	UpdateWindow(hwnd);
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
	RECT rect;
	GetClientRect(hwnd, &rect);
	return Rect4i(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
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
