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
		
		virtual void onClicked(Vector2f mousePos);
		virtual void onDoubleClicked(Vector2f mousePos);

		virtual void onRightClicked(Vector2f mousePos);
		virtual void onRightDoubleClicked(Vector2f mousePos);

		virtual void onMiddleClicked(Vector2f mousePos);
		
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
		bool held[3] = {false, false, false};
		bool forceUpdate = false;

		Vector2f clickPos[3] = {Vector2f(), Vector2f(), Vector2f()};
		Time clickTime[3] = { 100.0, 100.0, 100.0 };

		void onMouseClicked(Vector2f mousePos, int button);
		void onMouseDoubleClicked(Vector2f mousePos, int button);
	};
}
