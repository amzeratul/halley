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
		explicit UIList(const String& id, UIStyle style, UISizerType orientation = UISizerType::Vertical, int nColumns = 1);

		bool setSelectedOption(int option);
		bool setSelectedOptionId(const String& id);
		int getSelectedOption() const;
		String getSelectedOptionId() const;
		size_t getCount() const;

		UIStyle getStyle() const;

		void addTextItem(const String& id, const LocalisedString& label, float maxWidth = -1, bool centre = false);
		void addImage(const String& id, std::shared_ptr<UIImage> element, float proportion = 0, Vector4f border = {}, int fillFlags = UISizerFillFlags::Fill, Maybe<UIStyle> styleOverride = {});
		void addItem(const String& id, std::shared_ptr<IUIElement> element, float proportion = 0, Vector4f border = {}, int fillFlags = UISizerFillFlags::Fill, Maybe<UIStyle> styleOverride = {});
		void clear();

		void setItemEnabled(const String& id, bool enabled);
		void setItemActive(const String& id, bool active);

		Rect4f getOptionRect(int curOption) const;

		void onManualControlCycleValue(int delta) override;
		void onManualControlActivate() override;

		void readFromDataBind() override;

		std::shared_ptr<UIListItem> getItem(int n) const;
		std::shared_ptr<UIListItem> getItem(const String& id) const;
		
		bool canDrag() const;
		void setDrag(bool drag);

		void setUniformSizedItems(bool enabled);

		bool ignoreClip() const override;


	protected:
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;
		void onInput(const UIInputResults& input, Time time) override;

	private:
		UIStyle style;
		UISizerType orientation;
		Sprite sprite;
		std::vector<std::shared_ptr<UIListItem>> items;

		int curOptionHighlight = -1;
		int curOption = -1;
		int nColumns = 1;
		bool firstUpdate = true;
		bool dragEnabled = false;
		bool manualDragging = false;
		bool uniformSizedItems = false;

		void onItemClicked(UIListItem& item);
		void onItemDoubleClicked(UIListItem& item);
		void onItemDragged(UIListItem& item, int index, Vector2f pos);
		void addItem(std::shared_ptr<UIListItem> item);
		void onAccept();
		void onCancel();
		void reassignIds();
		size_t getNumberOfItems() const;

		void swapItems(int idxA, int idxB);
		bool isManualDragging() const;
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
		Rect4f getMouseRect() const override;
		Rect4f getRawRect() const;
		void notifySwap(Vector2f to);
		bool canSwap() const;
		Vector2f getOrigPosition() const;

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
		Sprite sprite;
		Vector4f extraMouseArea;
		bool selected = false;
		
		bool held = false;
		bool dragged = false;
		Vector2f mouseStartPos;
		Vector2f myStartPos;
		Vector2f origPos;
		Vector2f curDragPos;

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
