#pragma once
#include "../ui_widget.h"
#include "halley/core/input/input_keys.h"

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

		void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
		void releaseMouse(Vector2f mousePos, int button) override;

		void onClick(UIEventCallback callback);
		
		virtual void onClicked(Vector2f mousePos, KeyMods keyMods);
		virtual void onDoubleClicked(Vector2f mousePos, KeyMods keyMods);

		virtual void onRightClicked(Vector2f mousePos, KeyMods keyMods);
		virtual void onRightDoubleClicked(Vector2f mousePos, KeyMods keyMods);

		virtual void onMiddleClicked(Vector2f mousePos, KeyMods keyMods);
		
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
		std::pair<bool, KeyMods> held[3] = {{false, KeyMods::None}, {false, KeyMods::None}, {false, KeyMods::None}};
		bool forceUpdate = false;

		Vector2f clickPos[3] = {Vector2f(), Vector2f(), Vector2f()};
		Time clickTime[3] = { 100.0, 100.0, 100.0 };

		void onMouseClicked(Vector2f mousePos, int button, KeyMods keyMods);
		void onMouseDoubleClicked(Vector2f mousePos, int button, KeyMods keyMods);
	};
}
