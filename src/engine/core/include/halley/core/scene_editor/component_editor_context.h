#pragma once

#include <memory>

namespace Halley {
	class IUISizer;
	class ComponentFieldParameters;
	class UIWidget;
	class IEntityEditor;
	class UIFactory;
	class IUIElement;
	class Resources;
	class String;
	class ConfigNode;
    enum class SceneEditorTool;

	class IEntityEditor {
	public:
		virtual ~IEntityEditor() = default;

		virtual void onEntityUpdated() = 0;
		virtual std::shared_ptr<IUIElement> makeLabel(const String& label) = 0;
		virtual std::shared_ptr<IUIElement> createField(const String& fieldType, const ComponentFieldParameters& parameters, bool createLabel) = 0;
		virtual void setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options) = 0;
		virtual ConfigNode getDefaultNode(const String& fieldType) = 0;
	};
	
    class ComponentEditorContext {
    public:
	    ComponentEditorContext(IEntityEditor& parent, UIFactory& factory, Resources& gameResources);

        UIFactory& getUIFactory() const;
	    Resources& getGameResources() const;
        void onEntityUpdated() const;
	    void setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options) const;
    	
		std::shared_ptr<IUIElement> makeLabel(const String& label) const;
		std::shared_ptr<IUIElement> createField(const String& fieldType, const ComponentFieldParameters& parameters, bool createLabel) const;
    	ConfigNode getDefaultNode(const String& fieldType) const;

    private:
        IEntityEditor& parent;
        UIFactory& factory;
        Resources& gameResources;
    };
}