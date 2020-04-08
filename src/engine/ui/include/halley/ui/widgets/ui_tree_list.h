#pragma once

#include "ui_list.h"

namespace Halley {
    class UITreeListControls : public UIWidget {
    public:
        UITreeListControls(String id, UIStyle style);

        float updateGuides(const std::vector<int>& itemsLeftPerDepth, bool hasChildren);

    private:
    	UIStyle style;
    	std::vector<std::shared_ptr<UIImage>> guides;
        std::shared_ptr<UIImage> expandArrow;
    	bool waitingConstruction = true;
        float totalIndent = 0;
    };
	
    class UITreeListItem {
    public:
    	UITreeListItem();
    	UITreeListItem(String id, std::shared_ptr<UIListItem> listItem, std::shared_ptr<UITreeListControls> treeControls);

    	UITreeListItem* tryFindId(const String& id);
    	void addChild(UITreeListItem item);

    	void updateTree();

    private:
    	String id;
        std::shared_ptr<UIListItem> listItem;
        std::shared_ptr<UITreeListControls> treeControls;
    	std::vector<UITreeListItem> children;

    	void doUpdateTree(std::vector<int>& itemsLeftPerDepth);
    };
	
    class UITreeList : public UIList {
    public:
    	UITreeList(String id, UIStyle style);

        void addTreeItem(const String& id, const String& parentId, const LocalisedString& label);

    protected:
        void update(Time t, bool moved) override;
    	
    private:
    	UITreeListItem root;

    	UITreeListItem& getItemOrRoot(const String& id);
    };
}
