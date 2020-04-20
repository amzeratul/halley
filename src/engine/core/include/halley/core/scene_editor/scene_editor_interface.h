#pragma once

#include "halley/entity/entity_id.h"
#include "halley/time/halleytime.h"

namespace Halley {
	enum class SceneEditorTool;
	class UUID;
	class RenderContext;
    class World;
    class HalleyAPI;
    class Resources;
    class UIFactory;
    class IUIElement;
	class UIWidget;
	class IUISizer;
	class ConfigNode;
	class EntityRef;
	class Camera;
	class Painter;
	class ISceneEditorGizmoCollection;
	class ComponentFieldParameters;
	class ComponentEditorContext;

    enum class SceneEditorTool {
    	None,
        Drag,
        Translate,
        Rotate,
        Scale,
        Polygon
    };

    template <>
    struct EnumNames<SceneEditorTool> {
        constexpr std::array<const char*, 6> operator()() const {
            return
        	{{
				"none",
            	"drag",
            	"translate",
        		"rotate",
        		"scale",
        		"polygon"
            }};
        }
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

    class SceneEditorContext {
    public:
        const HalleyAPI* api;
        Resources* resources;
        Resources* editorResources;
        ISceneEditorGizmoCollection* gizmos;
    };

    class IComponentEditorFieldFactory {
    public:
        virtual ~IComponentEditorFieldFactory() = default;
        virtual String getFieldType() = 0;
        virtual bool canCreateLabel() const { return false; }
        virtual bool isNested() const { return false; }
        virtual void createLabelAndField(IUISizer& parent, const ComponentEditorContext& context, const ComponentFieldParameters& parameters) {}
        virtual std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& parameters) = 0;
    };

    class ISceneEditor {
    public:    	
        virtual ~ISceneEditor() = default;

        virtual void init(SceneEditorContext& context) = 0;
        virtual void update(Time t, SceneEditorInputState inputState, SceneEditorOutputState& outputState) = 0;
        virtual void render(RenderContext& rc) = 0;
    	
        virtual World& getWorld() const = 0;
        virtual void spawnPending() = 0;

        virtual EntityId getCameraId() = 0;
        virtual void dragCamera(Vector2f amount) = 0;
        virtual void changeZoom(int amount, Vector2f cursorPosRelToCamera) = 0;

    	virtual void setSelectedEntity(const UUID& id, ConfigNode& entityData) = 0;
        virtual void onEntityAdded(const UUID& id, const ConfigNode& entityData) = 0;
        virtual void onEntityRemoved(const UUID& id) = 0;
        virtual void onEntityModified(const UUID& id, const ConfigNode& entityData) = 0;
        virtual void onEntityMoved(const UUID& id, const ConfigNode& entityData) = 0;

    	virtual void showEntity(const UUID& id) = 0;
        virtual ConfigNode onToolSet(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options) = 0;

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

	class ISceneEditorGizmoCollection {
	public:
		virtual ~ISceneEditorGizmoCollection() = default;

        virtual void update(Time time, const Camera& camera, const SceneEditorInputState& inputState, SceneEditorOutputState& outputState) = 0;
        virtual void draw(Painter& painter) = 0;
        virtual void setSelectedEntity(const std::optional<EntityRef>& entity, ConfigNode& entityData) = 0;
        virtual std::shared_ptr<UIWidget> setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, const ConfigNode& options) = 0;
	};
}
