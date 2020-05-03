#pragma once
#include "../ui_widget.h"

namespace Halley {
	class UIStyle;
	class AudioClip;

	class UIClickable : public UIWidget {
	public:
		enum class State {
			Up,
			Down,
			Hover
		};

		UIClickable(String id, Vector2f minSize, std::optional<UISizer> sizer = {}, Vector4f innerBorder = {});

		bool canInteractWithMouse() const override;
		bool isFocusLocked() const override;

		void pressMouse(Vector2f mousePos, int button) override;
		void releaseMouse(Vector2f mousePos, int button) override;

		void onClick(UIEventCallback callback);
		virtual void onClicked(Vector2f mousePos) = 0;
		virtual void onDoubleClicked(Vector2f mousePos);

		void onGamepadInput(const UIInputResults& input, Time time) override;

		Rect4f getMouseRect() const override;
		void setMouseExtraBorder(std::optional<Vector4f> override);

	protected:

		void update(Time t, bool moved) override;
		virtual void onStateChanged(State prev, State next);
		bool setState(State state);
		virtual void doSetState(State state) = 0;
		State getCurState() const;
		bool updateButton();
		void doForceUpdate();
		void onEnabledChanged() override;
		virtual void onShortcutPressed();

	private:
		State curState = State::Up;
		std::optional<Vector4f> mouseExtraBorder;
		bool held = false;
		bool forceUpdate = false;

		Vector2f clickPos;
		Time clickTime = 100.0;
	};
}
