#pragma once
#include "../ui_widget.h"
#include "halley/core/graphics/sprite/sprite.h"

namespace Halley {
	class UIStyle;
	class AudioClip;

	class UIButton : public UIWidget {
	public:
		explicit UIButton(String id, std::shared_ptr<UIStyle> style);

		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;
		bool isFocusable() const override;
		bool isFocusLocked() const override;

		void pressMouse(int button) override;
		void releaseMouse(int button) override;

		void onClick(UIEventCallback callback);
		virtual void onClicked();

	protected:
		enum class State {
			Up,
			Down,
			Hover
		};

		virtual bool setState(State state);

		State curState = State::Up;
		Sprite sprite;
		std::shared_ptr<UIStyle> style;

	private:
		bool held = false;
	};
}
