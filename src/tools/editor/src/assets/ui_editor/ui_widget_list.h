#pragma once

#include <halley.hpp>

namespace Halley {
	class UIEditor;

	class UIWidgetList : public UIWidget {
    public:
        UIWidgetList(String id, UIFactory& factory);

        void onMakeUI() override;

        void setDefinition(std::shared_ptr<UIDefinition> definition);
        void setUIEditor(UIEditor& editor);
        void addWidget(const ConfigNode& node, const String& parentId, size_t childIdx = std::numeric_limits<size_t>::max());
        void doAddWidget(const ConfigNode& node, const String& parentId, size_t childIdx = std::numeric_limits<size_t>::max());
        void onWidgetModified(const String& id, const ConfigNode& data);

        UITreeList& getList();

    private:
        struct MoveOperation {
            String itemId;
            String parentId;
            int childIdx;
        };

        struct EntryInfo {
	        String label;
            Sprite icon;
            bool canHaveChildren = true;
        };

        UIFactory& factory;
        std::shared_ptr<UIDefinition> definition;
        std::shared_ptr<UITreeList> list;
        UIEditor* uiEditor = nullptr;

        void populateList();

        void moveItems(gsl::span<const MoveOperation> changes);
        EntryInfo getEntryInfo(const ConfigNode& data) const;
    };
}
