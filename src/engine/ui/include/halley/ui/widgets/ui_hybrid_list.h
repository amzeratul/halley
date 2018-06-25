#pragma once
#include "ui_list.h"

namespace Halley {
    class UIHybridList : public UIWidget {
    public:
		explicit UIHybridList(const String& id, UIStyle listStyle, UIStyle buttonStyle, UISizerType orientation = UISizerType::Vertical, int nColumns = 1);

		void addTextItem(const String& id, const LocalisedString& label);
	    void addDivider();

		void setInputButtons(const UIInputButtons& button);

    private:
		std::shared_ptr<UIList> list;
		std::shared_ptr<UIWidget> buttons;
		UIStyle listStyle;
		UIStyle buttonStyle;
		int nColumns;
    };
}
