#pragma once

#include "../ui_widget.h"

namespace Halley {
    enum class UIResizeDividerType {
        HorizontalLeft,
        HorizontalRight,
        VerticalTop,
        VerticalBottom
    };
    
	template <>
	struct EnumNames<UIResizeDividerType> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"horizontalLeft",
				"horizontalRight",
				"verticalTop",
				"verticalBottom"
			}};
		}
	};

	class UIResizeDivider : public UIWidget {
	public:
		UIResizeDivider(String id, UIResizeDividerType type);

		void update(Time t, bool moved) override;
		void onActiveChanged(bool active) override;

	protected:
		void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
		void releaseMouse(Vector2f mousePos, int button) override;
		void onMouseOver(Vector2f mousePos) override;
		void onAddedToRoot(UIRoot& root) override;
		std::optional<MouseCursorMode> getMouseCursorMode() const override;

		Rect4f getMouseRect() const override;

    private:
        UIResizeDividerType type;

		bool gotTarget = false;
		bool held = false;
		float startSize = 0;
		Vector2f startPos;

		std::shared_ptr<UIWidget> target;

		void acquireTarget();
		bool isHorizontal() const;
		bool isTargetBeforeMe() const;

		void setTargetSize(float size, bool store);
		void loadTargetSize();
    };
}
