#pragma once

#include <memory>

#include "halley/graphics/sprite/sprite.h"
#include "halley/maths/uuid.h"

namespace Halley {
	class IProjectWindow;
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

	class IEntityEditorCallbacks {
	public:
		virtual ~IEntityEditorCallbacks() = default;

		virtual void onEntityUpdated() = 0;
		virtual void reloadEntity() = 0;
		virtual void setTool(const String& tool, const String& componentName, const String& fieldName) = 0;
		virtual void setDefaultName(const String& name, const String& prevName) = 0;

		struct EntityInfo {
			String name;
			Sprite icon;
			UUID uuid;
		};

		virtual Vector<EntityInfo> getEntities() const { return {}; }
		virtual EntityInfo getEntityInfo(const UUID& uuid) const { return {}; }
		virtual void goToEntity(const UUID& uuid) {}
	};

	class IEntityEditor : public IEntityEditorCallbacks {
	public:
		virtual ~IEntityEditor() = default;

		virtual void addComponent(const String& name, ConfigNode data) = 0;
		virtual void deleteComponent(const String& name) = 0;
		virtual void onFieldChangedByGizmo(const String& componentName, const String& fieldName) = 0;
		virtual void onFieldChangedProcedurally(const String& componentName, const String& fieldName) = 0;
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
	    ComponentEditorContext(IProjectWindow& projectWindow, IEntityEditorFactory& entityEditorFactory, IEntityEditorCallbacks* entityEditor, UIFactory& factory, Resources* gameResources);

        UIFactory& getUIFactory() const;
	    Resources& getGameResources() const;
		IProjectWindow& getProjectWindow() const;
        void onEntityUpdated() const;
	    void setTool(const String& tool, const String& componentName, const String& fieldName) const;
    	
		std::shared_ptr<IUIElement> makeLabel(const String& label) const;
		std::shared_ptr<IUIElement> makeField(const String& fieldType, ComponentFieldParameters parameters, ComponentEditorLabelCreation createLabel) const;
    	ConfigNode getDefaultNode(const String& fieldType) const;
	    void setDefaultName(const String& name, const String& prevName) const;

		IEntityEditorCallbacks* getEntityEditorCallbacks() const;

    private:
		IProjectWindow& projectWindow;
    	IEntityEditorFactory* entityEditorFactory = nullptr;
        IEntityEditorCallbacks* entityEditor = nullptr;
        UIFactory* factory = nullptr;
        Resources* gameResources = nullptr;
    };
}
