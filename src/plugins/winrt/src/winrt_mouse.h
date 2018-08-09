#pragma once
#include "input/input_button_base.h"
#include <winrt/Windows.UI.Core.h>
#include "halley/data_structures/maybe.h"

namespace Halley {
	class WinRTMouse : public InputButtonBase {
	public:
		WinRTMouse();
		~WinRTMouse();

		void update();
		void setRemap(std::function<Vector2f(Vector2i)> remapFunction);

		Vector2f getPosition() const override;
		int getWheelMove() const override;

	private:
		Maybe<winrt::Windows::UI::Core::CoreWindow> window;
		std::function<Vector2f(Vector2i)> remap;
		Vector2i nativePos;
		
		winrt::event_token keyDownEvent;
		winrt::event_token keyUpEvent;
		winrt::event_token pointerWheelEvent;
		winrt::event_token pointerPressedEvent;
		winrt::event_token pointerReleasedEvent;
	};
}
