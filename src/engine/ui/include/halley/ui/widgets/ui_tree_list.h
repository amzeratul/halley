#pragma once

#include "ui_list.h"

namespace Halley {
    class UITreeList : public UIList {
    public:
    	UITreeList(String id, UIStyle style);

        void addTreeItem(const String& id, const String& parentId, const LocalisedString& label);
    };
}
