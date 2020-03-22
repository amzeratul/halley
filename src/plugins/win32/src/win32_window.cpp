#include "win32_window.h"

using namespace Halley;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//switch (uMsg) {
	//default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	//}
}

Win32Window::Win32Window(const WindowDefinition& def)
	: definition(def)
{
	auto hInstance = GetModuleHandle(nullptr);

	const static char* windowClassName = "HalleyWindow";
	WNDCLASS wc = {};
	wc.lpfnWndProc = WindowProc;
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
	DestroyWindow(hwnd);
}

void* Win32Window::getNativeHandle()
{
	return reinterpret_cast<void*>(hwnd);
}

String Win32Window::getNativeHandleType()
{
	return "Win32";
}
