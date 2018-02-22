#pragma once
#include "ui_clickable.h"

namespace Halley {
	class UIMenuButtonGroup;

	class UIMenuButton : public UIClickable {
	public:
		UIMenuButton(String id, Vector2f minSize = {}, Maybe<UISizer> sizer = {}, Vector4f innerBorder = {});

		void onClicked(Vector2f mousePos) override;
		void onOptionChosen();
		void setGroupFocused(bool focused);

	protected:
		void doSetState(State state) override final;
		virtual void onGroupState(State state);

	private:
		std::shared_ptr<UIMenuButtonGroup> group;
		bool groupFocused = false;
	};

	class UIMenuButtonGroup {
	public:
		void addButton(std::shared_ptr<UIMenuButton> button, const String& up = "", const String& down = "", const String& left = "", const String& right = "");
		void setCancelId(const String& id);

		void onInput(const UIInputResults& input);
		bool setFocus(UIMenuButton& uiMenuButton);
		bool setFocus(const String& id);
		std::shared_ptr<UIMenuButton> getCurrentFocus() const;

	private:
		struct ButtonEntry {
			std::weak_ptr<UIMenuButton> button;
			String id;
			String up;
			String down;
			String left;
			String right;
		};

		std::vector<ButtonEntry> buttons;
		String curFocus;
		String cancelId;

		const ButtonEntry& getCurFocusEntry() const;
		ButtonEntry& getCurFocusEntry();
	};

	class UIMenuButtonControlWidget : public UIWidget {
	public:
		UIMenuButtonControlWidget(std::shared_ptr<UIMenuButtonGroup> group);

	protected:
		void onInput(const UIInputResults& input) override;

	private:
		std::shared_ptr<UIMenuButtonGroup> group;
	};
}
