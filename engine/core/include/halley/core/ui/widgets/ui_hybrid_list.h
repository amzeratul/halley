#pragma once
#include "ui_list.h"

namespace Halley {
    class UIHybridList : public UIWidget {
    public:
		using AddCallback = std::function<std::shared_ptr<UIWidget>(const String&)>;

		explicit UIHybridList(const String& id, AddCallback callback, std::shared_ptr<UIStyle> style, UISizerType orientation = UISizerType::Vertical, int nColumns = 1, Maybe<Vector4f> innerBorder = {});

		void addId(const String& id);

		void setInputButtons(const UIList::Buttons& button);

    private:
		std::shared_ptr<UIList> list;
		std::shared_ptr<UIWidget> buttons;
		std::shared_ptr<UIStyle> style;
		AddCallback callback;
		int nColumns;
		Maybe<Vector4f> innerBorder;
    };
}
