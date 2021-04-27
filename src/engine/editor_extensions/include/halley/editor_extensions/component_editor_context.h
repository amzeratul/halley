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

	enum class ComponentEditorLabelCreation {
		Always,
		Never,
		OnlyIfNested
	};

	class IEntityEditor {
	public:
		virtual ~IEntityEditor() = default;

		virtual void onEntityUpdated() = 0;
		virtual std::shared_ptr<IUIElement> makeLabel(const String& label) = 0;
		virtual std::shared_ptr<IUIElement> makeField(const String& fieldType, ComponentFieldParameters parameters, ComponentEditorLabelCreation createLabel) = 0;
		virtual void setTool(const String& tool, const String& componentName, const String& fieldName) = 0;
		virtual ConfigNode getDefaultNode(const String& fieldType) = 0;
		virtual void setDefaultName(const String& name, const String& prevName) = 0;
	};
	
    class ComponentEditorContext {
    public:
	    ComponentEditorContext(IEntityEditor& parent, UIFactory& factory, Resources& gameResources);

        UIFactory& getUIFactory() const;
	    Resources& getGameResources() const;
        void onEntityUpdated() const;
	    void setTool(const String& tool, const String& componentName, const String& fieldName) const;
    	
		std::shared_ptr<IUIElement> makeLabel(const String& label) const;
		std::shared_ptr<IUIElement> makeField(const String& fieldType, ComponentFieldParameters parameters, ComponentEditorLabelCreation createLabel) const;
    	ConfigNode getDefaultNode(const String& fieldType) const;
	    void setDefaultName(const String& name, const String& prevName) const;

    private:
        IEntityEditor& parent;
        UIFactory& factory;
        Resources& gameResources;
    };
}