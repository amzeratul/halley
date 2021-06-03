#pragma once

#include "halley/time/halleytime.h"
#include "halley/file_formats/config_file.h"
#include "halley/text/halleystring.h"
#include "halley/maths/uuid.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/text/i18n.h"
#include <optional>
#include "halley/core/input/input_keyboard.h"

namespace Halley {
	class IEntityEditorFactory;
	class EntityFactory;
	class ScriptNodeTypeCollection;
	class Sprite;
	class LocalisedString;
	class Task;
	class Prefab;
	class ISceneEditorWindow;
	class UIDebugConsoleController;
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
	class UIColourScheme;
	class SceneEditorGizmo;
	class UIList;
    struct EntityId;
	struct SceneEditorInputState;
	struct SceneEditorOutputState;
	struct SnapRules;
	
    enum class EditorSettingType {
        Editor,
        Project,
        Temp
    };
	
	class IEditorInterface {
	public:
		virtual ~IEditorInterface() = default;

		virtual bool saveAsset(const Path& path, gsl::span<const gsl::byte> data) = 0;
		virtual void addTask(std::unique_ptr<Task> task) = 0;

		virtual const ConfigNode& getSetting(EditorSettingType type, std::string_view id) const = 0;
		virtual void setSetting(EditorSettingType type, std::string_view id, ConfigNode data) = 0;
		virtual const ConfigNode& getAssetSetting(std::string_view id) const = 0;
		virtual void setAssetSetting(std::string_view id, ConfigNode data) = 0;
	};

    class SceneEditorContext {
    public:
        const HalleyAPI* api;
        Resources* resources;
        Resources* editorResources;
        ISceneEditorGizmoCollection* gizmos;
    	IEditorInterface* editorInterface;
    };

    class IComponentEditorFieldFactory {
    public:
        virtual ~IComponentEditorFieldFactory() = default;
        virtual String getFieldType() = 0;
        virtual bool canCreateLabel() const { return false; }
        virtual bool isNested() const { return false; }
        virtual std::shared_ptr<IUIElement> createLabelAndField(const ComponentEditorContext& context, const ComponentFieldParameters& parameters) { return {}; }
        virtual std::shared_ptr<IUIElement> createField(const ComponentEditorContext& context, const ComponentFieldParameters& parameters) = 0;
        virtual ConfigNode getDefaultNode() const { return ConfigNode(); }
    };

    class ISceneEditor {
    public:    	
        virtual ~ISceneEditor() = default;

        virtual void init(SceneEditorContext& context) = 0;
        virtual void update(Time t, SceneEditorInputState inputState, SceneEditorOutputState& outputState) = 0;
        virtual void render(RenderContext& rc) = 0;

    	virtual bool isReadyToCreateWorld() const = 0;
    	virtual void createWorld(std::shared_ptr<const UIColourScheme> colourScheme) = 0;
    	
        virtual World& getWorld() const = 0;
    	virtual Resources& getResources() const = 0;
        virtual void spawnPending() = 0;

        virtual const std::vector<EntityId>& getCameraIds() const = 0;
        virtual void dragCamera(Vector2f amount) = 0;
    	virtual void moveCamera(Vector2f pos) = 0;
    	virtual bool loadCameraPos() = 0;
        virtual void changeZoom(int amount, Vector2f cursorPosRelToCamera) = 0;

    	virtual void setSelectedEntity(const UUID& id, EntityData& entityData) = 0;
    	virtual void setEntityHighlightedOnList(const UUID& id) = 0;
        virtual void onEntityAdded(const UUID& id, const EntityData& entityData) = 0;
        virtual void onEntityRemoved(const UUID& id) = 0;
        virtual void onEntityModified(const UUID& id, const EntityData& entityData) = 0;
        virtual void onEntityMoved(const UUID& id, const EntityData& entityData) = 0;

    	virtual void showEntity(const UUID& id) = 0;
        virtual void onToolSet(String& tool, String& componentName, String& fieldName) = 0;

    	virtual std::vector<std::unique_ptr<IComponentEditorFieldFactory>> getComponentEditorFieldFactories() = 0;
    	virtual std::shared_ptr<UIWidget> makeCustomUI() = 0;
    	virtual void setupConsoleCommands(UIDebugConsoleController& controller, ISceneEditorWindow& sceneEditor) = 0;
        virtual void onSceneLoaded(Prefab& scene) = 0;
    	virtual void onSceneSaved() = 0;
        virtual void refreshAssets() = 0;

    	virtual void setupTools(UIList& toolList, ISceneEditorGizmoCollection& gizmoCollection) = 0;

    	virtual Rect4f getSpriteTreeBounds(const EntityRef& e) const = 0;

    	virtual void cycleHighlight(int delta) = 0;

    	virtual std::optional<Vector2f> getMousePos() const = 0;
    	virtual Vector2f getCameraPos() const = 0;

    	virtual std::shared_ptr<ScriptNodeTypeCollection> getScriptNodeTypes() = 0;
    	
        virtual std::vector<std::pair<String, String>> getRightClickMenu(const Vector2f& mousePos) const = 0;
    };

	class EntityTree {
	public:
		String entityId;
		String name;
		String prefab;
		String icon;
		std::vector<EntityTree> children;

		bool contains(const String& id) const
		{
			if (entityId == id) {
				return true;
			}

			for (const auto& child: children) {
				if (child.contains(id)) {
					return true;
				}
			}

			return false;
		}
	};

	class ISceneData {
	public:
		class ConstEntityNodeData;
		
        class EntityNodeData {
        	friend class ConstEntityNodeData;
        public:
            EntityNodeData(EntityData& data, String parentId) : data(data), parentId(std::move(parentId)) {}

        	EntityData& getData() const { return data; }
        	const String& getParentId() const { return parentId; }

        private:
	        EntityData& data;
        	String parentId;
        };

		class ConstEntityNodeData {
        public:
            ConstEntityNodeData(EntityData& data, String parentId) : data(data), parentId(std::move(parentId)) {}
			ConstEntityNodeData(EntityNodeData&& other) noexcept : data(other.data), parentId(std::move(other.parentId)) {}
        	const EntityData& getData() const { return data; }
        	const String& getParentId() const { return parentId; }

        private:
	        EntityData& data;
        	String parentId;
        };
		
		virtual ~ISceneData() = default;

		virtual EntityNodeData getWriteableEntityNodeData(const String& id) = 0;
		virtual ConstEntityNodeData getEntityNodeData(const String& id) = 0;
		virtual void reloadEntity(const String& id) = 0;
		virtual EntityTree getEntityTree() const = 0;
		virtual std::pair<String, size_t> reparentEntity(const String& entityId, const String& newParentId, size_t childIndex) = 0;
        virtual bool isSingleRoot() = 0;
	};

	class ISceneEditorGizmoCollection {
	public:
		struct Tool {
			String id;
			LocalisedString toolTip;
			Sprite icon;
			KeyboardKeyPress shortcut;

			Tool() = default;
			Tool(String id, LocalisedString toolTip, Sprite icon, KeyboardKeyPress shortcut)
				: id(std::move(id)), toolTip(std::move(toolTip)), icon(std::move(icon)), shortcut(shortcut)
			{}
		};

		using GizmoFactory = std::function<std::unique_ptr<SceneEditorGizmo>(SnapRules snapRules, const String& componentName, const String& fieldName)>;
		
		virtual ~ISceneEditorGizmoCollection() = default;

        virtual bool update(Time time, const Camera& camera, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState, SceneEditorOutputState& outputState) = 0;
        virtual void draw(Painter& painter) = 0;
        virtual void setSelectedEntity(const std::optional<EntityRef>& entity, EntityData& entityData) = 0;
		virtual void refreshEntity() = 0;
        virtual std::shared_ptr<UIWidget> setTool(const String& tool, const String& componentName, const String& fieldName) = 0;
		virtual void deselect() = 0;
		virtual void addTool(const Tool& tool, GizmoFactory gizmoFactory) = 0;
		virtual void generateList(UIList& list) = 0;
		virtual ISceneEditorWindow& getSceneEditorWindow() = 0;
	};
	
	class ISceneEditorWindow {
	public:
		virtual ~ISceneEditorWindow() = default;

		virtual void markModified() = 0;
		
		virtual void onEntityAdded(const String& id, const String& parentId, int childIndex) = 0;
		virtual void onEntityRemoved(const String& id, const String& parentId, int childIndex, const EntityData& prevData) = 0;
		virtual void onEntityModified(const String& id, const EntityData& prevData, const EntityData& newData) = 0;
		virtual void onEntityMoved(const String& id, const String& prevParentId, int prevChildIndex, const String& newParentId, int newChildIndex) = 0;
		virtual void onComponentRemoved(const String& name) = 0;
		virtual void onFieldChangedByGizmo(const String& componentName, const String& fieldName) = 0;

		virtual void removeEntity(const String& entityId) = 0;
		
		virtual const std::shared_ptr<ISceneData>& getSceneData() const = 0;

		virtual void addComponentToCurrentEntity(const String& componentName) = 0;
		virtual void setHighlightedComponents(std::vector<String> componentNames) = 0;
		virtual const IEntityEditorFactory& getEntityEditorFactory() const = 0;

		virtual std::shared_ptr<ScriptNodeTypeCollection> getScriptNodeTypes() = 0;

		virtual const ConfigNode& getSetting(EditorSettingType type, std::string_view id) const = 0;
		virtual void setSetting(EditorSettingType type, std::string_view id, ConfigNode data) = 0;

		virtual float getProjectDefaultZoom() const = 0;

		virtual std::shared_ptr<EntityFactory> getEntityFactory() const = 0;
		virtual void spawnUI(std::shared_ptr<UIWidget> ui) = 0;
	};

	class IProject {
    public:		
        virtual ~IProject() = default;
		virtual Path getAssetsSrcPath() const = 0;
	};

	class IProjectWindow {
	public:
		virtual const ConfigNode& getSetting(EditorSettingType type, std::string_view id) const = 0;
        virtual void setSetting(EditorSettingType type, std::string_view id, ConfigNode data) = 0;
	};

    class IEditorCustomTools { 
    public:
        struct ToolData {
            String id;
            LocalisedString text;
        	LocalisedString tooltip;
            Sprite icon;
            std::shared_ptr<UIWidget> widget;

        	ToolData(String id, LocalisedString text, LocalisedString tooltip, Sprite icon, std::shared_ptr<UIWidget> widget)
                : id(std::move(id))
                , text(std::move(text))
        		, tooltip(std::move(tooltip))
                , icon(std::move(icon))
                , widget(std::move(widget))
            {}
        };

        struct MakeToolArgs {
        	UIFactory& factory;
            Resources& editorResources;
            Resources& gameResources;
            const HalleyAPI& api;
        	IProject& project;
        	IProjectWindow& projectWindow;

            MakeToolArgs(UIFactory& factory, Resources& editorResources, Resources& gameResources, const HalleyAPI& api, IProject& project, IProjectWindow& projectWindow)
                : factory(factory)
                , editorResources(editorResources)
        		, gameResources(gameResources)
                , api(api)
        		, project(project)
        		, projectWindow(projectWindow)
            {}
        };

        virtual ~IEditorCustomTools() = default;

        virtual std::vector<ToolData> makeTools(const MakeToolArgs& args) = 0;
    };
}
