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
		Sprite icon;
		bool isSeparator = false;
		bool enabled = true;

		UIPopupMenuItem(String id, LocalisedString text, LocalisedString tooltip)
			: id(std::move(id))
			, text(std::move(text))
			, tooltip(std::move(tooltip))
		{			
		}

		UIPopupMenuItem(String id, LocalisedString text, Sprite icon, LocalisedString tooltip)
			: id(std::move(id))
			, text(std::move(text))
			, tooltip(std::move(tooltip))
			, icon(icon)
		{			
		}

		UIPopupMenuItem(String id, String tooltip)
			: id(id)
			, text(LocalisedString::fromHardcodedString(std::move(id)))
			, tooltip(LocalisedString::fromHardcodedString(std::move(tooltip)))
		{
		}

		UIPopupMenuItem()
			: isSeparator(true)
		{}
	};

	class UIPopupMenu : public UIWidget {
	public:
		UIPopupMenu(String id, UIStyle style, std::vector<UIPopupMenuItem> items);

		void update(Time t, bool moved) override;
		
		void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;

		void onAddedToRoot(UIRoot& root) override;
		void setInputButtons(const UIInputButtons& buttons) override;
		void spawnOnRoot(UIRoot& uiRoot);

	private:
		UIStyle style;
		UIInputButtons inputButtons;
		
		std::vector<UIPopupMenuItem> items;
		std::shared_ptr<UIList> itemList;

		bool destroyOnUpdate = false;

		void makeUI();
	};
}
