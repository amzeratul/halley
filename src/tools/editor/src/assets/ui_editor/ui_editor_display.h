#pragma once

#include <halley.hpp>

namespace Halley {
	class UIEditor;

	class UIEditorDisplay : public UIWidget, IUIElement::IUIElementListener {
	public:
		UIEditorDisplay(String id, Vector2f minSize, UISizer sizer);

		void setUIEditor(UIEditor& uiEditor);

		void drawAfterChildren(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void setSelectedWidget(const String& id);
		void loadDisplay(const UIDefinition& uiDefinition);

		void onPlaceInside(Rect4f rect, const std::shared_ptr<IUIElement>& element, UISizer& sizer) override;

	private:
		UIEditor* editor;
		std::map<UUID, std::shared_ptr<UIWidget>> widgets;
		std::shared_ptr<const UIWidget> curWidget;
		int maxAdjustment = 0;

		Sprite boundsSprite;
		Sprite sizerSprite;
		std::vector<Sprite> sizerSprites;
		std::map<UISizer*, std::vector<Rect4f>> sizerRects;
		UISizer* curSizer = nullptr;

		void makeSizerSprites();
	};
}
