#pragma once

namespace Halley {
	class UIEditor;

	class UIWidgetList : public UIWidget {
    public:
        UIWidgetList(String id, UIFactory& factory);

        void onMakeUI() override;

        void setDefinition(std::shared_ptr<UIDefinition> definition);
        void setUIEditor(UIEditor& editor);
        void addWidget(const ConfigNode& node, String parentId);

        UITreeList& getList();

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

        void moveItems(gsl::span<const MoveOperation> changes);
    };
}
