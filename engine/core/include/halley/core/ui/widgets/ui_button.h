#pragma once
#include "../ui_widget.h"
#include "halley/core/graphics/sprite/sprite.h"

namespace Halley {
	class UIStyle;
	class AudioClip;

	class UIClickable : public UIWidget {
	public:
		explicit UIClickable(String id, Vector2f minSize, Maybe<UISizer> sizer = {}, Vector4f innerBorder = {});

		bool isFocusable() const override;
		bool isFocusLocked() const override;

		void pressMouse(Vector2f mousePos, int button) override;
		void releaseMouse(Vector2f mousePos, int button) override;

		void onClick(UIEventCallback callback);
		virtual void onClicked(Vector2f mousePos) = 0;

		void setInputButton(int button);
		void updateInputDevice(InputDevice& device) override;

	protected:
		enum class State {
			Up,
			Down,
			Hover
		};

		bool setState(State state);
		virtual void doSetState(State state) = 0;
		State getCurState() const;
		bool updateButton();
		void doForceUpdate();

	private:
		State curState = State::Up;
		bool held = false;
		bool forceUpdate = false;
		int inputButton = -1;
	};


	class UIButton : public UIClickable {
	public:
		explicit UIButton(String id, std::shared_ptr<UIStyle> style);

		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;
		void onClicked(Vector2f mousePos) override;
		void setInputType(UIInputType uiInput) override;

		bool isFocusable() const override;
		bool isFocusLocked() const override;

	protected:
		void doSetState(State state) override;

	private:
		Sprite sprite;
		std::shared_ptr<UIStyle> style;
		UIInputType curInputType = UIInputType::Undefined;
		bool borderOnly = false;
	};
}
