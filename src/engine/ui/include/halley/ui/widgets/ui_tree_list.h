#pragma once

#include "ui_list.h"
#include "ui_label.h"

namespace Halley {
    class UITreeListControls : public UIWidget {
    public:
        UITreeListControls(String id, UIStyle style);

        float updateGuides(const std::vector<int>& itemsLeftPerDepth, bool hasChildren, bool expanded);
        void setExpanded(bool expanded);

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
        enum class PositionType {
        	OnTop,
	        Before,
        	After,
        	End
        };

    	struct FindPositionResult {
    		PositionType type;
    		const UITreeListItem* item;
    		Rect4f rect;

    		FindPositionResult() = default;
    		FindPositionResult(PositionType type, const UITreeListItem* item, Rect4f rect)
                : type(type)
    			, item(item)
    			, rect(rect)
            {}
    	};
    	
    	UITreeListItem();
    	UITreeListItem(String id, std::shared_ptr<UIListItem> listItem, std::shared_ptr<UITreeListControls> treeControls, std::shared_ptr<UILabel> label, bool forceLeaf);

    	UITreeListItem* tryFindId(const String& id);
    	void addChild(std::unique_ptr<UITreeListItem> item, const String& afterSiblingId);
        void addChild(std::unique_ptr<UITreeListItem> item, size_t pos);
    	std::unique_ptr<UITreeListItem> removeChild(const String& id);
        void moveChild(size_t oldChildIndex, size_t newChildIndex);

        void setLabel(const LocalisedString& label);
        void setExpanded(bool expanded);

        std::unique_ptr<UITreeListItem> removeFromTree(const String& id);
        void updateTree(UITreeList& treeList);
    	void collectItems(std::vector<std::shared_ptr<UIListItem>>& items);
    	std::optional<FindPositionResult> findPosition(UITreeList& tree, Vector2f pos) const;
    	
        const String& getId() const;
    	const String& getParentId() const;
    	size_t getNumberOfChildren() const;
        size_t getChildIndex(const String& id) const;

        std::shared_ptr<UIListItem> getListItem() const;
        const std::vector<std::unique_ptr<UITreeListItem>>& getChildren() const;

    	bool canHaveChildren() const;

    private:
    	String id;
    	String parentId;
        std::shared_ptr<UIListItem> listItem;
        std::shared_ptr<UILabel> label;
        std::shared_ptr<UITreeListControls> treeControls;
    	std::vector<std::unique_ptr<UITreeListItem>> children;
    	bool expanded = true;
    	bool forceLeaf = false;

    	void doUpdateTree(UITreeList& treeList, std::vector<int>& itemsLeftPerDepth, bool treeExpanded);
        std::optional<FindPositionResult> doFindPosition(UITreeList& tree, Vector2f pos, int depth, bool lasBranch) const;
    };
	
    class UITreeList : public UIList {
    public:
    	UITreeList(String id, UIStyle style);

        void addTreeItem(const String& id, const String& parentId, const String& afterSiblingId, const LocalisedString& label, const String& labelStyle = "label", bool forceLeaf = false);
        void removeItem(const String& id);
        void setLabel(const String& id, const LocalisedString& label);

        void clear() override;
        void sortItems();

    	void setSingleRoot(bool enabled);
    	bool isSingleRoot() const;

        bool canDragListItem(const UIListItem& listItem) override;
    	
    protected:
        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;
        void onItemDragging(UIListItem& item, int index, Vector2f pos) override;
        void onItemDoneDragging(UIListItem& item, int index, Vector2f pos) override;
    	
    private:
    	UITreeListItem root;
    	Sprite insertCursor;
    	bool needsRefresh = true;
    	bool singleRoot = false;

    	UITreeListItem& getItemOrRoot(const String& id);
        void setupEvents();
    	void reparentItem(const String& id, const String& newParentId, int childIndex);
    	void removeTree(const UITreeListItem& tree);
    };
}
