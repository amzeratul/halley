#pragma once

namespace Halley {
	class ProjectWindow;
	class EntityEditorFactory;
    class UIEditor;

	class UIWidgetEditor : public UIWidget, private IEntityEditorCallbacks {
    public:
        UIWidgetEditor(String id, UIFactory& factory);

    	void setSelectedWidget(const String& id, ConfigNode* node);
        void setGameResources(Resources& resources);
        void setUIEditor(UIEditor& uiEditor, ProjectWindow& projectWindow);

    protected:
        void onEntityUpdated() override;
        void reloadEntity() override;
        void setTool(const String& tool, const String& componentName, const String& fieldName) override;
        void setDefaultName(const String& name, const String& prevName) override;
        IProjectWindow& getProjectWindow() const override;

    private:
        UIFactory& factory;
        String curId;
        ConfigNode* curNode = nullptr;
        UIEditor* uiEditor = nullptr;
        ProjectWindow* projectWindow = nullptr;
        std::shared_ptr<EntityEditorFactory> entityFieldFactory;

        void refresh();
        void populateWidgetBox(UIWidget& root, ConfigNode& widgetNode, const UIFactoryWidgetProperties& properties);
        void populateGenericWidgetBox(UIWidget& root, ConfigNode& widgetNode);
        void populateFillBox(UIWidget& root, ConfigNode& widgetNode);
        void populateSizerBox(UIWidget& root, ConfigNode& widgetNode);

        void populateBox(UIWidget& root, ConfigNode& node, gsl::span<const UIFactoryWidgetProperties::Entry> entries);
    };
}
