#pragma once

namespace Halley {
	class EntityEditorFactory;

	class UIWidgetEditor : public UIWidget, private IEntityEditorCallbacks {
    public:
        UIWidgetEditor(String id, UIFactory& factory);

    	void setSelectedWidget(const String& id, ConfigNode* node);
        void setGameResources(Resources& resources);

	protected:
        void onEntityUpdated() override;
        void reloadEntity() override;
        void setTool(const String& tool, const String& componentName, const String& fieldName) override;
        void setDefaultName(const String& name, const String& prevName) override;
        ISceneEditorWindow& getSceneEditorWindow() const override;

    private:
        UIFactory& factory;
        ConfigNode* curNode = nullptr;
        std::shared_ptr<EntityEditorFactory> entityFieldFactory;

        struct Entry {
            String label;
	        String name;
            String type;
            std::vector<String> defaultValue;
        };

        void refresh();
        void populateWidgetBox(UIWidget& root, ConfigNode& widgetNode);
        void populateFillBox(UIWidget& root, ConfigNode& widgetNode);
        void populateSizerBox(UIWidget& root, ConfigNode& widgetNode);

        void populateBox(UIWidget& root, ConfigNode& node, gsl::span<Entry> entries);
    };
}
