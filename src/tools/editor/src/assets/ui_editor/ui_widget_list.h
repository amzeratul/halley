#pragma once

namespace Halley {
    class UIWidgetList : public UIWidget {
    public:
        UIWidgetList(String id, UIFactory& factory);

        void onMakeUI() override;

        void setDefinition(std::shared_ptr<const UIDefinition> definition);

    private:
        UIFactory& factory;
        std::shared_ptr<const UIDefinition> definition;
        std::shared_ptr<UITreeList> list;

        void populateList();
        void populateList(const ConfigNode& curNode, String parentId);
    };
}
