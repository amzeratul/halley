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
        std::shared_ptr<UIButton> expandButton;
        std::shared_ptr<UIButton> collapseButton;
    	bool waitingConstruction = true;
        float totalIndent = 0;
        size_t lastDepth = 0;
    	
    	void setupUI();
    };

	class UITreeList;
	
    class UITreeListItem {
    public:
    	UITreeListItem();
    	UITreeListItem(String id, std::shared_ptr<UIListItem> listItem, std::shared_ptr<UITreeListControls> treeControls);

    	UITreeListItem* tryFindId(const String& id);
    	void addChild(UITreeListItem item);

    	void updateTree(UITreeList& treeList);
        void setExpanded(bool expanded);

    private:
    	String id;
        std::shared_ptr<UIListItem> listItem;
        std::shared_ptr<UITreeListControls> treeControls;
    	std::vector<UITreeListItem> children;
    	bool expanded = true;

    	void doUpdateTree(UITreeList& treeList, std::vector<int>& itemsLeftPerDepth, bool treeExpanded);
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
        void setupEvents();
    };
}
