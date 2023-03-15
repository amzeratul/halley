#pragma once

#include "ui_list.h"
#include "ui_label.h"

namespace Halley {
    class UITreeListControls : public UIWidget {
    public:
        UITreeListControls(String id, UIStyle style);

        float updateGuides(const Vector<int>& itemsLeftPerDepth, bool hasChildren, bool expanded);
        void setExpanded(bool expanded);

    private:
    	Vector<std::shared_ptr<UIImage>> guides;
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
    	UITreeListItem(String id, std::shared_ptr<UIListItem> listItem, std::shared_ptr<UITreeListControls> treeControls, std::shared_ptr<UILabel> label, std::shared_ptr<UIImage> iconWidget, bool forceLeaf, bool expanded);

    	UITreeListItem* tryFindId(const String& id);
        UITreeListItem& addChild(std::unique_ptr<UITreeListItem> item, size_t pos);
    	std::unique_ptr<UITreeListItem> removeChild(const String& id);
        void moveChild(size_t oldChildIndex, size_t newChildIndex);

        void setLabel(const LocalisedString& label);
        void setLabelColour(Colour4f colour);
        Colour4f getLabelColour() const;
    	void setIcon(Sprite icon);
        bool setExpanded(bool expanded);
        bool setAllExpanded(UITreeList& tree, bool expanded);
        void setForceLeaf(bool leaf);

        std::unique_ptr<UITreeListItem> removeFromTree(const String& id);
        void updateTree(UITreeList& treeList);
    	std::optional<FindPositionResult> findPosition(UITreeList& tree, Vector2f pos) const;

    	void collectItems(Vector<std::shared_ptr<UIListItem>>& items);
        void enumerateIdsAndLabels(Vector<String>& ids, Vector<String>& names, Vector<Sprite>& icons) const;

        const String& getId() const;
    	const String& getParentId() const;
    	size_t getNumberOfChildren() const;
        size_t getChildIndex(const String& id) const;

        std::shared_ptr<UIListItem> getListItem() const;
        const Vector<std::unique_ptr<UITreeListItem>>& getChildren() const;

    	bool canHaveChildren() const;

        std::pair<bool, bool> expandParentsOfId(const String& targetId); // returns {containsId, modified}
        std::optional<String> getLastExpandedItem(const String& targetId);

        bool addTag(String tag);
        bool removeTag(const String& tag);
        bool hasTag(const String& tag) const;
        bool hasTagInAncestors(const String& tag) const;

    private:
    	String id;
    	String parentId;
        UITreeListItem* parent = nullptr;
        std::shared_ptr<UIListItem> listItem;
        std::shared_ptr<UILabel> label;
    	std::shared_ptr<UIImage> icon;
        std::shared_ptr<UITreeListControls> treeControls;
    	Vector<std::unique_ptr<UITreeListItem>> children;
        Vector<String> tags;
    	bool expanded = true;
    	bool forceLeaf = false;

    	void doUpdateTree(UITreeList& treeList, Vector<int>& itemsLeftPerDepth, bool treeExpanded);
        std::optional<FindPositionResult> doFindPosition(UITreeList& tree, Vector2f pos, int depth, bool lasBranch) const;
        std::optional<String> doGetLastExpandedItem(bool expandedTree, const String& lastId, const String& id);
    };
	
    class UITreeList : public UIList {
    public:
    	UITreeList(String id, UIStyle style);

        UITreeListItem& addTreeItem(const String& id, const String& parentId, size_t childIndex, const LocalisedString& label, const String& labelStyle = "label", Sprite icon = Sprite(), bool forceLeaf = false, bool expanded = true);
        void removeItem(const String& id, bool immediate = true);
        void setLabel(const String& id, const LocalisedString& label, Sprite icon);
        void setForceLeaf(const String& id, bool forceLeaf);

        bool setSelectedOptionIds(gsl::span<const String> ids, SelectionMode mode = SelectionMode::Normal) override;

        UITreeListItem* tryGetTreeItem(const String& id);

        void clear() override;
        void sortItems();

    	void setSingleRoot(bool enabled);
    	bool isSingleRoot() const;

        void setAllExpanded(bool expanded);

        bool canDragListItem(const UIListItem& listItem) override;

        void makeParentsOfItemExpanded(const String& id);
        bool setSelectedOptionId(const String& id, SelectionMode mode = SelectionMode::Normal) override;

        void enumerateIdsAndLabels(Vector<String>& ids, Vector<String>& names, Vector<Sprite>& icons) const;

        Vector2f getDragPositionAdjustment(Vector2f pos, Vector2f startPos) const override;
    	void refresh();

    protected:
        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;
        void onItemDragging(UIListItem& item, int index, Vector2f pos) override;
        void onItemDoneDragging(UIListItem& item, int index, Vector2f pos) override;

    	virtual bool canParentItemTo(const String& itemId, const String& parentId) const;
        virtual bool canDragItemId(const String& itemId) const;
    	
    private:
        friend class UITreeListItem;

    	UITreeListItem root;
    	Sprite insertCursor;
    	bool needsRefresh = true;
    	bool singleRoot = false;

    	UITreeListItem& getItemOrRoot(const String& id);
        void setupEvents();
    	void reparentItems(gsl::span<const String> ids, const String& newParentId, int childIndex);
    	void removeTree(const UITreeListItem& tree);
    };
}
