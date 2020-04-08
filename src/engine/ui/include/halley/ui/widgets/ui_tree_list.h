#pragma once

#include "ui_list.h"

namespace Halley {
    class UITreeListControls : public UIWidget {
    public:
        UITreeListControls(String id, Sprite elementImage);

        void setDepth(int depth);

    private:
        std::shared_ptr<UIWidget> spacer;
        std::shared_ptr<UIImage> elementImage;
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

    	void doUpdateTree(int depth);
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
