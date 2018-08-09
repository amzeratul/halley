#include "winrt_mouse.h"
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.System.h>
using namespace Halley;

using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::System;

WinRTMouse::WinRTMouse()
	: InputButtonBase(3)
{
	remap = [](Vector2i pos) { return Vector2f(pos); };
	window = CoreWindow::GetForCurrentThread();

	keyDownEvent = window->KeyDown([=] (CoreWindow window, const KeyEventArgs& args)
	{
		switch (args.VirtualKey()) {
		case VirtualKey::MiddleButton:
			onButtonPressed(1);
			args.Handled(true);
			break;
		case VirtualKey::RightButton:
			onButtonPressed(2);
			args.Handled(true);
			break;
		default:
			args.Handled(false);
		}
	});

	keyUpEvent = window->KeyDown([=] (CoreWindow window, const KeyEventArgs& args)
	{
		switch (args.VirtualKey()) {
		case VirtualKey::MiddleButton:
			onButtonReleased(1);
			args.Handled(true);
			break;
		case VirtualKey::RightButton:
			onButtonReleased(2);
			args.Handled(true);
			break;
		default:
			args.Handled(false);
		}
	});

	pointerPressedEvent = window->PointerPressed([=] (CoreWindow window, const PointerEventArgs& args)
	{
		onButtonPressed(0);
		args.Handled(true);
	});

	pointerReleasedEvent = window->PointerReleased([=] (CoreWindow window, const PointerEventArgs& args)
	{
		onButtonReleased(0);
		args.Handled(true);
	});

	pointerWheelEvent = window->PointerWheelChanged([=] (CoreWindow window, const PointerEventArgs& args)
	{
		args.Handled(true);
	});
}

WinRTMouse::~WinRTMouse()
{
	window->KeyDown(keyDownEvent);
	window->KeyUp(keyUpEvent);
	window->PointerWheelChanged(pointerWheelEvent);
	window->PointerPressed(pointerPressedEvent);
	window->PointerReleased(pointerReleasedEvent);
}

void WinRTMouse::update()
{
	auto pos = window->PointerPosition();
	auto bounds = window->Bounds();
	//auto windowPos = Vector2f((pos.X - bounds.X) / float(bounds.Width), (pos.Y - bounds.Y) / float(bounds.Height));
	//nativePos = Vector2i(windowPos * Vector2f(1920, 1080));
	nativePos = Vector2i(pos.X - bounds.X, pos.Y - bounds.Y);
}

void WinRTMouse::setRemap(std::function<Vector2f(Vector2i)> remapFunction)
{
	remap = std::move(remapFunction);
}

Vector2f WinRTMouse::getPosition() const
{
	return remap(nativePos);
}

int WinRTMouse::getWheelMove() const
{
	// TODO
	return 0;
}
