#pragma once
#include "ui_list.h"

namespace Halley {
    class UIHybridList : public UIWidget {
    public:
		enum class AddType {
			Button,
			List
		};

		using AddCallback = std::function<std::shared_ptr<UIWidget>(const String&, AddType)>;

		explicit UIHybridList(const String& id, AddCallback callback, UIStyle listStyle, UIStyle buttonStyle, UISizerType orientation = UISizerType::Vertical, int nColumns = 1);

		void addId(const String& id);
	    void addDivider();

		void setInputButtons(const UIInputButtons& button);

    private:
		std::shared_ptr<UIList> list;
		std::shared_ptr<UIWidget> buttons;
		UIStyle listStyle;
		UIStyle buttonStyle;
		AddCallback callback;
		int nColumns;
    };
}
