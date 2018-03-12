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

		UIClickable(String id, Vector2f minSize, Maybe<UISizer> sizer = {}, Vector4f innerBorder = {});

		bool canInteractWithMouse() const override;
		bool isFocusLocked() const override;

		void pressMouse(Vector2f mousePos, int button) override;
		void releaseMouse(Vector2f mousePos, int button) override;

		void onClick(UIEventCallback callback);
		virtual void onClicked(Vector2f mousePos) = 0;

		void onInput(const UIInputResults& input) override;

	protected:

		bool setState(State state);
		virtual void doSetState(State state) = 0;
		State getCurState() const;
		bool updateButton();
		void doForceUpdate();
		void onEnabledChanged() override;

	private:
		State curState = State::Up;
		bool held = false;
		bool forceUpdate = false;
	};
}
