#pragma once

#include <halley.hpp>

#include "src/ui/base_canvas.h"

namespace Halley {
	class UIEditor;

	class UIEditorDisplay : public UIWidget, IUIElement::IUIElementListener, IUIReloadObserver {
	public:
		UIEditorDisplay(String id, Vector2f minSize, UISizer sizer, const HalleyAPI& api, Resources& resources);

		void setUIEditor(UIEditor* uiEditor);

		void drawAfterChildren(UIPainter& painter) const override;
		void update(Time t, bool moved) override;
		void onLayout() override;

		void setSelectedWidget(const String& id);
		void clearDisplay();
		void loadDisplay(const UIDefinition& uiDefinition);

		void onPlaceInside(Rect4f rect, Rect4f origRect, const std::shared_ptr<IUIElement>& element, UISizer& sizer) override;
		void applyTransform(const Matrix4f& matrix) override;

		void setZoom(float zoom);
		bool ignoreClip() const override;
		Rect4f getCurWidgetRect() const;

		void setCanvas(std::shared_ptr<BaseCanvas> canvas);

	private:
		UIEditor* editor = nullptr;
		std::map<UUID, std::shared_ptr<IUIElement>> elements;
		std::shared_ptr<const IUIElement> curElement;
		int maxAdjustment = 0;

		String curSelection;

		Rect4f curRect;
		std::map<UISizer*, Vector<std::pair<Rect4f, bool>>> sizerRects;
		UISizer* curSizer = nullptr;

		std::shared_ptr<UIRenderSurface> displayRoot;

		Matrix4f transform;
		std::shared_ptr<InputKeyboard> keyboard;
		std::shared_ptr<UIWidget> lastWidgetUnderMouse;

		std::shared_ptr<BaseCanvas> canvas;
		bool processingMouse = false;

		void updateCurWidget();
		void doLayout();
		void onOtherUIReloaded(UIWidget& ui) override;
		void notifyWidgetUnderMouse(const std::shared_ptr<UIWidget>& widget) override;

		std::optional<std::shared_ptr<UIWidget>> prePressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
		void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
		UUID getUUIDOfWidgetClicked(const UIWidget& widget) const;

	private:
		Rect4f transformRect(Rect4f r) const;
	};
}
