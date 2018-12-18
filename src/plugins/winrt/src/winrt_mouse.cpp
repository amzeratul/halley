#include "winrt_mouse.h"
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Input.h>
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
		case VirtualKey::LeftButton:
			onButtonPressed(0);
			args.Handled(true);
			break;
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
		case VirtualKey::LeftButton:
			onButtonReleased(0);
			args.Handled(true);
			break;
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
		// // This is called the first time you press any mouse button
		if (args.CurrentPoint().Properties().IsLeftButtonPressed()) {
			onButtonPressed(0);
		} else if (args.CurrentPoint().Properties().IsMiddleButtonPressed()) {
			onButtonPressed(1);
		} else if (args.CurrentPoint().Properties().IsRightButtonPressed()) {
			onButtonPressed(2);
		}
		args.Handled(true);
	});

	pointerReleasedEvent = window->PointerReleased([=] (CoreWindow window, const PointerEventArgs& args)
	{
		// This is called ONLY when all mouse buttons are released (not called for each release action)
		onButtonReleased(0);
		onButtonReleased(1);
		onButtonReleased(2);
		args.Handled(true);
	});

	pointerWheelEvent = window->PointerWheelChanged([=] (CoreWindow window, const PointerEventArgs& args)
	{
		args.Handled(true);
	});

	pointerMovedEvent = window->PointerMoved([=](CoreWindow window, const PointerEventArgs& args)
	{
		// Handles secondary pressed/released states while any mouse button is pressed
		switch (args.CurrentPoint().Properties().PointerUpdateKind())
		{
		case winrt::Windows::UI::Input::PointerUpdateKind::LeftButtonPressed:
			onButtonPressed(0);
			args.Handled(true);
			break;
		case winrt::Windows::UI::Input::PointerUpdateKind::MiddleButtonPressed:
			onButtonPressed(1);
			args.Handled(true);
			break;
		case winrt::Windows::UI::Input::PointerUpdateKind::RightButtonPressed:
			onButtonPressed(2);
			args.Handled(true);
			break;
		case winrt::Windows::UI::Input::PointerUpdateKind::LeftButtonReleased:
			onButtonReleased(0);
			args.Handled(true);
			break;
		case winrt::Windows::UI::Input::PointerUpdateKind::MiddleButtonReleased:
			onButtonReleased(1);
			args.Handled(true);
			break;
		case winrt::Windows::UI::Input::PointerUpdateKind::RightButtonReleased:
			onButtonReleased(2);
			args.Handled(true);
			break;
		default:
			args.Handled(false);
		}
	});

	// Initialise internals
	update();
}

WinRTMouse::~WinRTMouse()
{
	window->KeyDown(keyDownEvent);
	window->KeyUp(keyUpEvent);
	window->PointerWheelChanged(pointerWheelEvent);
	window->PointerPressed(pointerPressedEvent);
	window->PointerReleased(pointerReleasedEvent);
	window->PointerMoved(pointerMovedEvent);
}

void WinRTMouse::update()
{
	clearPresses();

	auto pos = window->PointerPosition();
	auto bounds = window->Bounds();
	//auto windowPos = Vector2f((pos.X - bounds.X) / float(bounds.Width), (pos.Y - bounds.Y) / float(bounds.Height));
	//nativePos = Vector2i(windowPos * Vector2f(1920, 1080));
	nativePos = Vector2i((int)(pos.X - bounds.X), (int)(pos.Y - bounds.Y));
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
