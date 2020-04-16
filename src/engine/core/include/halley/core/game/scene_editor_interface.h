#pragma once

#include "halley/entity/entity_id.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class UUID;
	class RenderContext;
    class World;
    class HalleyAPI;
    class Resources;
    class UIFactory;
    class IUIElement;
	class ConfigNode;
    enum class SceneEditorTool;

    class SceneEditorContext {
    public:
        const HalleyAPI* api;
        Resources* resources;
        Resources* editorResources;
    };

	class IEntityEditor {
	public:
		virtual ~IEntityEditor() = default;

		virtual void onEntityUpdated() = 0;
		virtual std::shared_ptr<IUIElement> makeLabel(const String& label) = 0;
		virtual void setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, const ConfigNode& options) = 0;
	};

    class ComponentEditorContext {
    public:
        ComponentEditorContext(IEntityEditor& parent, UIFactory& factory, Resources& gameResources)
            : parent(parent)
    		, factory(factory)
            , gameResources(gameResources)
        {}

        UIFactory& getFactory() const { return factory; }
        Resources& getGameResources() const { return gameResources; }
    	void onEntityUpdated() const { parent.onEntityUpdated(); }
    	std::shared_ptr<IUIElement> makeLabel(const String& label) const { return parent.makeLabel(label); }
        void setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, const ConfigNode& options) const { parent.setTool(tool, componentName, fieldName, options); }

    private:
        IEntityEditor& parent;
        UIFactory& factory;
        Resources& gameResources;
    };

	struct ComponentFieldParameters {
        ComponentFieldParameters(const String& componentName, const String& fieldName, const String& defaultValue, ConfigNode& componentData)
            : componentName(componentName)
			, fieldName(fieldName)
			, defaultValue(defaultValue)
			, componentData(componentData)
        {}
		
		const String& componentName;
		const String& fieldName;
		const String& defaultValue;
        ConfigNode& componentData;
    };

	class UIWidget;

    class IComponentEditorFieldFactory {
    public:
        virtual ~IComponentEditorFieldFactory() = default;
    	virtual String getFieldType() = 0;
    	virtual bool canCreateLabel() const { return false; }
    	virtual bool isCompound() const { return false; }
        virtual void createLabelAndField(UIWidget& parent, const ComponentEditorContext& context, const ComponentFieldParameters& parameters) {}
        virtual std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& parameters) = 0;
    };

    enum class SceneEditorTool {
    	None,
        Drag,
        Translate,
        Rotate,
        Scale,
        Polygon
    };

	struct SceneEditorInputState {
		// Filled on editor side
		bool leftClickPressed = false;
		bool leftClickHeld = false;
		bool middleClickPressed = false;
		bool middleClickHeld = false;
		bool rightClickPressed = false;
		bool rightClickHeld = false;
		bool shiftHeld = false;
		bool ctrlHeld = false;
		Vector2f rawMousePos;
		Rect4f viewRect;

		// Filled on SceneEditor side
        Vector2f mousePos;
    };

	struct SceneEditorOutputState {
		std::vector<std::pair<String, String>> fieldsChanged;
	};

    class ISceneEditor {
    public:    	
        virtual ~ISceneEditor() = default;

        virtual void init(SceneEditorContext& context) = 0;
        virtual void update(Time t, SceneEditorInputState inputState, SceneEditorOutputState& outputState) = 0;
        virtual void render(RenderContext& rc) = 0;
    	
        virtual World& getWorld() = 0;
        virtual void spawnPending() = 0;

        virtual EntityId getCameraId() = 0;
        virtual void dragCamera(Vector2f amount) = 0;
        virtual void changeZoom(int amount, Vector2f cursorPosRelToCamera) = 0;

    	virtual void setSelectedEntity(const UUID& id, ConfigNode& entityData) = 0;
    	virtual void showEntity(const UUID& id) = 0;
    	virtual void setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, const ConfigNode& options) = 0;

    	virtual std::vector<std::unique_ptr<IComponentEditorFieldFactory>> getComponentEditorFieldFactories() = 0;
    	virtual std::shared_ptr<UIWidget> makeCustomUI() = 0;
    };

	class EntityTree {
	public:
		String entityId;
		String name;
		String prefab;
		std::vector<EntityTree> children;
	};

	class ISceneData {
	public:
        class EntityData {
        public:
            EntityData(ConfigNode& data, String parentId)
                : data(data)
        		, parentId(std::move(parentId))
            { }
        	
	        ConfigNode& data;
        	String parentId;
        };
		
		virtual ~ISceneData() = default;

		virtual EntityData getEntityData(const String& id) = 0;
		virtual void reloadEntity(const String& id) = 0;
		virtual EntityTree getEntityTree() const = 0;
		virtual void reparentEntity(const String& entityId, const String& newParentId, int childIndex) = 0;
        virtual bool isSingleRoot() = 0;
	};
}
