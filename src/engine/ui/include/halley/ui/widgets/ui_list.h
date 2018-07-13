#pragma once

#include "../ui_widget.h"
#include "../ui_style.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
#include "ui_button.h"

namespace Halley {
	class UIStyle;
	class UIListItem;

	class UIList : public UIWidget {
		friend class UIListItem;

	public:
		explicit UIList(const String& id, UIStyle style, UISizerType orientation = UISizerType::Vertical, int nColumns = 1);

		bool setSelectedOption(int option);
		bool setSelectedOptionId(const String& id);
		int getSelectedOption() const;
		String getSelectedOptionId() const;
		size_t getCount() const;

		void addTextItem(const String& id, const LocalisedString& label, float maxWidth = -1, bool centre = false);
		void addItem(const String& id, std::shared_ptr<UIWidget> widget, float proportion = 0, Vector4f border = {}, int fillFlags = UISizerFillFlags::Fill, Maybe<UIStyle> styleOverride = boost::none);
		void addItem(const String& id, std::shared_ptr<UISizer> sizer, float proportion = 0, Vector4f border = {}, int fillFlags = UISizerFillFlags::Fill, Maybe<UIStyle> styleOverride = boost::none);
		void clear();

		void setItemEnabled(const String& id, bool enabled);
		void setItemActive(const String& id, bool active);

		Rect4f getOptionRect(int curOption) const;

		void onManualControlCycleValue(int delta) override;
		void onManualControlActivate() override;

		void readFromDataBind() override;

		std::shared_ptr<UIListItem> getItem(int n) const;

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

		void onItemClicked(UIListItem& item);
		void addItem(std::shared_ptr<UIListItem> item);
		void onAccept();
		void onCancel();
		void reassignIds();
		size_t getNumberOfItems() const;
	};

	class UIListItem : public UIClickable {
	public:
		explicit UIListItem(const String& id, UIList& parent, UIStyle style, int index, Vector4f extraMouseArea);

		void onClicked(Vector2f mousePos) override;
		void setSelected(bool selected);
		
		int getIndex() const;
		void setIndex(int index);
		Rect4f getMouseRect() const override;
		Rect4f getRawRect() const;

	protected:
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

	private:
		UIList& parent;
		UIStyle style;
		int index;
		Sprite sprite;
		Vector4f extraMouseArea;
		bool selected = false;

		void doSetState(State state) override;
		void updateSpritePosition();
	};
}
