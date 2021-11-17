#pragma once

namespace Halley {
	class UIEditor;

	class UIWidgetList : public UIWidget {
    public:
        UIWidgetList(String id, UIFactory& factory);

        void onMakeUI() override;

        void setDefinition(std::shared_ptr<UIDefinition> definition);
        void setUIEditor(UIEditor& editor);

    private:
        struct MoveOperation {
            String itemId;
            String parentId;
            int childIdx;
        };

        UIFactory& factory;
        std::shared_ptr<UIDefinition> definition;
        std::shared_ptr<UITreeList> list;
        UIEditor* uiEditor;

        void populateList();
        void populateList(const ConfigNode& curNode, String parentId);

        void moveItems(gsl::span<const MoveOperation> changes);
    };
}
