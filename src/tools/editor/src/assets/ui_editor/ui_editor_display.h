#pragma once

#include <halley.hpp>

namespace Halley {
	class UIEditor;

	class UIEditorDisplay : public UIWidget {
	public:
		UIEditorDisplay(String id, Vector2f minSize, UISizer sizer);

		void setUIEditor(UIEditor& uiEditor);

		void drawAfterChildren(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void setSelectedWidget(const String& id);
		void loadDisplay(const UIDefinition& uiDefinition);

	private:
		UIEditor* editor;
		std::map<UUID, std::shared_ptr<UIWidget>> widgets;
		std::shared_ptr<UIWidget> curWidget;
		int maxAdjustment = 0;

		Sprite boundsSprite;
	};
}
