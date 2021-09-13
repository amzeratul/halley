#pragma once

#include <memory>

namespace Halley {
	class ISceneEditorWindow;
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
		virtual void setTool(const String& tool, const String& componentName, const String& fieldName) = 0;
		virtual void setDefaultName(const String& name, const String& prevName) = 0;
		virtual ISceneEditorWindow& getSceneEditorWindow() const = 0;
	};

	class IEntityEditorFactory {
	public:
		virtual ~IEntityEditorFactory() = default;

		virtual std::shared_ptr<IUIElement> makeLabel(const String& label) const = 0;
		virtual std::shared_ptr<IUIElement> makeField(const String& fieldType, ComponentFieldParameters parameters, ComponentEditorLabelCreation createLabel) const = 0;
		virtual ConfigNode getDefaultNode(const String& fieldType) const = 0;
	};
	
    class ComponentEditorContext {
    public:
	    ComponentEditorContext(IEntityEditorFactory& entityEditorFactory, IEntityEditor* entityEditor, UIFactory& factory, Resources& gameResources);

        UIFactory& getUIFactory() const;
	    Resources& getGameResources() const;
		ISceneEditorWindow& getSceneEditorWindow() const;
        void onEntityUpdated() const;
	    void setTool(const String& tool, const String& componentName, const String& fieldName) const;
    	
		std::shared_ptr<IUIElement> makeLabel(const String& label) const;
		std::shared_ptr<IUIElement> makeField(const String& fieldType, ComponentFieldParameters parameters, ComponentEditorLabelCreation createLabel) const;
    	ConfigNode getDefaultNode(const String& fieldType) const;
	    void setDefaultName(const String& name, const String& prevName) const;

    private:
    	IEntityEditorFactory& entityEditorFactory;
        IEntityEditor* entityEditor = nullptr;
        UIFactory& factory;
        Resources& gameResources;
    };
}