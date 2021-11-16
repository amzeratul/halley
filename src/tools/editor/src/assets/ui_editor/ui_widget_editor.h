#pragma once

namespace Halley {
    class UIWidgetEditor : public UIWidget {
    public:
        UIWidgetEditor(String id, UIFactory& factory);

    	void setSelectedWidget(const String& id, ConfigNode* node);

    private:
        UIFactory& factory;
        ConfigNode* curNode = nullptr;

        void refresh();
        void populateWidgetBox(UIWidget& root, ConfigNode& widgetNode);
    };
}
