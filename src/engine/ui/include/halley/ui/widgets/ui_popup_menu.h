#pragma once

#include "../ui_widget.h"

namespace Halley {
	class UIFactory;
	class UIList;

	struct UIPopupMenuItem
	{
		String id;
		LocalisedString text;
		LocalisedString tooltip;

		UIPopupMenuItem(String id, LocalisedString text, LocalisedString tooltip)
			: id(std::move(id))
			, text(std::move(text))
			, tooltip(std::move(tooltip))
		{
			
		}

		UIPopupMenuItem(String id, String tooltip)
			: id(id)
			, text(LocalisedString::fromHardcodedString(std::move(id)))
			, tooltip(LocalisedString::fromHardcodedString(std::move(tooltip)))
		{

		}
	};

	class UIPopupMenu : public UIWidget {
	public:
		UIPopupMenu(String id, UIStyle style, std::vector<UIPopupMenuItem> items);

		void update(Time t, bool moved) override;
		
		void pressMouse(Vector2f mousePos, int button) override;

		void onAddedToRoot(UIRoot& root) override;
		void setInputButtons(const UIInputButtons& buttons) override;
		
	private:
		UIStyle style;
		UIInputButtons inputButtons;
		
		std::vector<UIPopupMenuItem> items;
		std::shared_ptr<UIList> itemList;

		bool destroyOnUpdate = false;

		void makeUI();
	};
}
