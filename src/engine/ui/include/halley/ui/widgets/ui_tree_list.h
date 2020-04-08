#pragma once

#include "ui_list.h"

namespace Halley {
    class UITreeListControls : public UIWidget {
    public:
        UITreeListControls(String id, Sprite elementSprite, UIStyle style);

        float setDepth(size_t depth);

    private:
    	UIStyle style;
    	Sprite elementSprite;
    	std::vector<std::shared_ptr<UIImage>> guides;
        std::shared_ptr<UIImage> elementImage;
    	bool waitingConstruction = true;
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
