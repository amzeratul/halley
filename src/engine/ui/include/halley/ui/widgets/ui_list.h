#pragma once

#include "../ui_widget.h"
#include "../ui_style.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
#include "ui_button.h"

namespace Halley {
	class UIStyle;
	class UIListItem;
	class UIImage;

	class UIList : public UIWidget {
		friend class UIListItem;

	public:
		explicit UIList(String id, UIStyle style, UISizerType orientation = UISizerType::Vertical, int nColumns = 1);

		bool setSelectedOption(int option);
		bool setSelectedOptionId(const String& id);
		int getSelectedOption() const;
		String getSelectedOptionId() const;
		size_t getCount() const;

		UIStyle getStyle() const;

		std::shared_ptr<UIListItem> addTextItem(const String& id, LocalisedString label, float maxWidth = -1, bool centre = false, std::optional<LocalisedString> tooltip = {});
		std::shared_ptr<UIListItem> addImage(const String& id, std::shared_ptr<UIImage> element, float proportion = 0, Vector4f border = {}, int fillFlags = UISizerFillFlags::Fill, std::optional<UIStyle> styleOverride = {});
		std::shared_ptr<UIListItem> addItem(const String& id, std::shared_ptr<IUIElement> element, float proportion = 0, Vector4f border = {}, int fillFlags = UISizerFillFlags::Fill, std::optional<UIStyle> styleOverride = {});
		std::optional<int> removeItem(const String& id);

		void clear() override;

		void setItemEnabled(const String& id, bool enabled);
		void setItemActive(const String& id, bool active);
		void filterOptions(const String& filter);

		Rect4f getOptionRect(int curOption) const;

		void onManualControlCycleValue(int delta) override;
		void onManualControlActivate() override;

		void readFromDataBind() override;

		std::shared_ptr<UIListItem> getItem(int n) const;
		std::shared_ptr<UIListItem> getItem(const String& id) const;
		std::shared_ptr<UIListItem> tryGetItem(const String& id) const;
		std::shared_ptr<UIListItem> getItemUnderCursor() const;

		bool isDragEnabled() const;
		void setDragEnabled(bool drag);
		bool isDragOutsideEnabled() const;
		void setDragOutsideEnabled(bool dragOutside);
		virtual bool canDragListItem(const UIListItem& listItem);

		bool isSingleClickAccept() const;
		void setSingleClickAccept(bool enabled);

		void setUniformSizedItems(bool enabled);

        void setScrollToSelection(bool enabled);

		bool ignoreClip() const override;

		bool canReceiveFocus() const override;
		void setFocusable(bool focusable);

		std::shared_ptr<UILabel> makeLabel(String id, LocalisedString label, float maxWidth = 0) const;

	protected:
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;
		
		void onGamepadInput(const UIInputResults& input, Time time) override;
		bool onKeyPress(KeyboardKeyPress key) override;
		void moveSelection(int dx, int dy);
				
		std::shared_ptr<UIListItem> addItem(std::shared_ptr<UIListItem> item, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill);
		size_t getNumberOfItems() const;

		virtual void onItemDragging(UIListItem& item, int index, Vector2f pos);
		virtual void onItemDoneDragging(UIListItem& item, int index, Vector2f pos);
		void reassignIds();

		UIStyle style;
		std::vector<std::shared_ptr<UIListItem>> items;
		int curOption = -1;

	private:
		UISizerType orientation;
		Sprite sprite;

		int nColumns = 1;
		bool firstUpdate = true;
		bool dragEnabled = false;
		bool dragOutsideEnabled = false;
		bool manualDragging = false;
		bool uniformSizedItems = false;
		bool singleClickAccept = true;
		bool focusable = true;
		bool scrollToSelection = true;
		int itemUnderCursor = -1;

		void onItemClicked(UIListItem& item);
		void onItemDoubleClicked(UIListItem& item);
		void onAccept();
		void onCancel();

		void swapItems(int idxA, int idxB);
		bool isManualDragging() const;
		void setItemUnderCursor(int itemIdx, bool isMouseOver);
	};

	class UIListItem : public UIClickable {
	public:
		explicit UIListItem(const String& id, UIList& parent, UIStyle style, int index, Vector4f extraMouseArea);

		void onClicked(Vector2f mousePos) override;
		void onDoubleClicked(Vector2f mousePos) override;
		void setSelected(bool selected);

		void setStyle(UIStyle style);

		int getIndex() const;
		void setIndex(int index);

		int getAbsoluteIndex() const;
		void setAbsoluteIndex(int index);
		
		Rect4f getMouseRect() const override;
		Rect4f getRawRect() const;
		
		void setClickableInnerBorder(Vector4f innerBorder);
		Vector4f getClickableInnerBorder() const;
		
		void notifySwap(Vector2f to);
		bool canSwap() const;
		Vector2f getOrigPosition() const;

		void setDraggableSubWidget(UIWidget* widget);
		
	protected:
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void onMouseOver(Vector2f mousePos) override;
		void pressMouse(Vector2f mousePos, int button) override;
		void releaseMouse(Vector2f mousePos, int button) override;
		void setDragPos(Vector2f pos);
		void onEnabledChanged() override;

	private:
		UIList& parent;
		UIStyle style;
		int index;
		int absoluteIndex;
		Sprite sprite;
		Vector4f extraMouseArea;
		Vector4f innerBorder;
		bool selected = false;

		bool held = false;
		bool dragged = false;
		Vector2f mouseStartPos;
		Vector2f myStartPos;
		Vector2f origPos;
		Vector2f curDragPos;
		Vector2f dragWidgetOffset;
		UIWidget* dragWidget = nullptr;

		bool swapping = false;
		Vector2f swapFrom;
		Vector2f swapTo;
		Time swapTime = 0;
		int manualDragTime = 0;

		void doSetState(State state) override;
		void updateSpritePosition();
		bool isManualDragging() const;
	};
}
