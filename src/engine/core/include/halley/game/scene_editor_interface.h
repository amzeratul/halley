#pragma once

#include "halley/time/halleytime.h"
#include "halley/file_formats/config_file.h"
#include "halley/text/halleystring.h"
#include "halley/maths/uuid.h"
#include "halley/graphics/sprite/sprite.h"
#include "halley/text/i18n.h"
#include <optional>
#include "halley/input/input_keyboard.h"
#include "halley/concurrency/future.h"

class Transform2DComponent;

namespace Halley {
	class UIRoot;
	struct AssetPreviewData;
	class IEntityEditor;
	class Image;
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
	class EntityIcons;
	class AssetPreviewGenerator;
	struct UIPopupMenuItem;
    struct EntityId;
	struct SceneEditorInputState;
	struct SceneEditorOutputState;
	struct SnapRules;
	
    class EntityValidator;

    class IEntityValidator {
    public:
        struct Action {
	        LocalisedString label;
            ConfigNode actionData;

			Action() = default;

			Action(LocalisedString label, ConfigNode data)
				: label(std::move(label))
				, actionData(std::move(data))
			{}

        	Action(String label, ConfigNode data)
        		: label(LocalisedString::fromUserString(label))
				, actionData(std::move(data))
			{}

            bool operator==(const Action& other) const;
            bool operator!=(const Action& other) const;
        };

    	enum class Severity {
    		None,
    		Warning,
    		Error
    	};

        struct Result {
            LocalisedString errorMessage;
            Vector<Action> suggestedActions;
        	Severity severity = Severity::Error;

			Result() = default;

			Result(Severity severity, String errorMessage)
				: errorMessage(LocalisedString::fromUserString(errorMessage))
        		, severity(severity)
			{}

			Result(Severity severity, String errorMessage, Action suggestedAction)
				: errorMessage(LocalisedString::fromUserString(errorMessage))
        		, severity(severity)
			{
				suggestedActions.push_back(std::move(suggestedAction));
			}

			Result(Severity severity, String errorMessage, Vector<Action> suggestedActions)
				: errorMessage(LocalisedString::fromUserString(errorMessage))
				, suggestedActions(std::move(suggestedActions))
        		, severity(severity)
			{
			}

			Result(Severity severity, LocalisedString errorMessage, Action suggestedAction)
				: errorMessage(std::move(errorMessage))
        		, severity(severity)
			{
				suggestedActions.push_back(std::move(suggestedAction));
			}

			Result(Severity severity, LocalisedString errorMessage, Vector<Action> suggestedActions)
				: errorMessage(std::move(errorMessage))
				, suggestedActions(std::move(suggestedActions))
        		, severity(severity)
			{
			}

            bool operator==(const Result& other) const;
            bool operator!=(const Result& other) const;
        };

    	virtual ~IEntityValidator() = default;

        virtual Vector<Result> validateEntity(EntityValidator& validator, const EntityData& currentEntityData) = 0;
    };

    class IEntityValidatorActionHandler {
    public:
        virtual ~IEntityValidatorActionHandler() = default;

        virtual bool canHandle(const ConfigNode& actionData) = 0;
        virtual void applyAction(EntityValidator& validator, IEntityEditor& entityEditor, EntityData& entityData, const ConfigNode& actionData) = 0;
        virtual bool canApplyAction(EntityValidator& validator, const IEntityEditor& entityEditor, const EntityData& entityData, const ConfigNode& actionData) = 0;
    };

	enum class EditorSettingType {
        Editor,
        Project,
        Temp
    };
	
	class IEditorInterface {
	public:
		virtual ~IEditorInterface() = default;

		virtual bool saveAsset(const Path& path, gsl::span<const gsl::byte> data) = 0;
		virtual void openAsset(AssetType assetType, const String& assetId) = 0;
		virtual void openAssetHere(AssetType assetType, const String& assetId) = 0;
		virtual void setAssetSaveNotification(bool enabled) = 0;
		virtual void addTask(std::unique_ptr<Task> task) = 0;

		virtual const ConfigNode& getSetting(EditorSettingType type, std::string_view id) const = 0;
		virtual void setSetting(EditorSettingType type, std::string_view id, ConfigNode data) = 0;
		virtual const ConfigNode& getAssetSetting(std::string_view id) const = 0;
		virtual void setAssetSetting(std::string_view id, ConfigNode data) = 0;
		virtual const ConfigNode& getAssetSetting(std::string_view assetKey, std::string_view id) const = 0;
		virtual void setAssetSetting(std::string_view assetKey, std::string_view id, ConfigNode data) = 0;
		virtual String getAssetKey() = 0;

		virtual void selectEntity(const String& uuid) = 0;
		virtual Sprite getEntityIcon(const String& uuid) = 0;
		virtual Sprite getAssetIcon(AssetType type) = 0;
		virtual void clearAssetCache() = 0;

		virtual void validateAllEntities() = 0;
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

        virtual const Vector<EntityId>& getCameraIds() const = 0;
        virtual void dragCamera(Vector2f amount) = 0;
    	virtual void moveCamera(Vector2f pos) = 0;
    	virtual bool loadCameraPos() = 0;
        virtual void changeZoom(int amount, Vector2f cursorPosRelToCamera) = 0;
		virtual float getZoom() const = 0;

    	virtual void setSelectedEntities(Vector<UUID> uuids, Vector<EntityData*> datas) = 0;
    	virtual void setEntityHighlightedOnList(const UUID& id, bool forceShow) = 0;

    	virtual void showEntity(const UUID& id) = 0;
        virtual void onToolSet(String& tool, String& componentName, String& fieldName) = 0;

    	virtual std::shared_ptr<UIWidget> makeCustomUI() = 0;
    	virtual void setupConsoleCommands(UIDebugConsoleController& controller, ISceneEditorWindow& sceneEditor) = 0;
        virtual void onSceneLoaded(Prefab& scene) = 0;
    	virtual void onSceneSaved() = 0;
        virtual void refreshAssets() = 0;

    	virtual void setupTools(UIList& toolList, ISceneEditorGizmoCollection& gizmoCollection) = 0;

    	virtual void cycleHighlight(int delta) = 0;

    	virtual std::optional<Vector2f> getMousePos() const = 0;
    	virtual Vector2f getCameraPos() const = 0;
        virtual std::optional<Vector2f> getWorldOffset() const = 0;

        virtual Vector<UIPopupMenuItem> getSceneContextMenu(const Vector2f& mousePos) const = 0;
        virtual void onSceneContextMenuSelection(const String& id) = 0;
    	virtual void onSceneContextMenuHighlight(const String& id) = 0;
    	
        virtual void setAssetPreviewGenerator(AssetPreviewGenerator& generator) = 0;

    	virtual Transform2DComponent* getTransform(const String& entityId) = 0;

    	virtual void initializeEntityValidator(EntityValidator& validator) = 0;
        virtual bool shouldDrawOutline(const Sprite& sprite) const = 0;
        virtual String getSceneNameForComments(AssetType assetType, const String& assetId) const = 0;
    };

	class EntityTree {
	public:
		String entityId;
		const EntityData* data = nullptr;
		Vector<EntityTree> children;

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
            EntityNodeData(EntityData& data, String parentId, int childIndex) : data(data), parentId(std::move(parentId)), childIndex(childIndex) {}

        	EntityData& getData() const { return data; }
        	const String& getParentId() const { return parentId; }
        	int getChildIndex() const { return childIndex; }

        private:
	        EntityData& data;
        	String parentId;
			int childIndex;
        };

		class ConstEntityNodeData {
        public:
            ConstEntityNodeData(EntityData& data, String parentId, int childIndex) : data(data), parentId(std::move(parentId)), childIndex(childIndex) {}
			ConstEntityNodeData(EntityNodeData&& other) noexcept : data(other.data), parentId(std::move(other.parentId)), childIndex(other.childIndex) {}
			
        	const EntityData& getData() const { return data; }
        	const String& getParentId() const { return parentId; }
			int getChildIndex() const { return childIndex; }

        private:
	        EntityData& data;
        	String parentId;
			int childIndex;
        };
		
		virtual ~ISceneData() = default;

		virtual std::optional<EntityNodeData> tryGetWriteableEntityNodeData(const String& id) = 0;
		virtual std::optional<ConstEntityNodeData> tryGetEntityNodeData(const String& id) = 0;

		EntityNodeData getWriteableEntityNodeData(const String& id)
		{
			auto result = tryGetWriteableEntityNodeData(id);
			if (!result) {
				throw Exception("Entity data not found for \"" + id + "\"", HalleyExceptions::Entity);
			}
			return *result;
		}
        ConstEntityNodeData getEntityNodeData(const String& id)
		{
			auto result = tryGetEntityNodeData(id);
			if (!result) {
				throw Exception("Entity data not found for \"" + id + "\"", HalleyExceptions::Entity);
			}
			return *result;
		}
		
		virtual std::pair<Vector<UUID>, Vector<EntityData*>> getWriteableEntityDatas(gsl::span<const UUID> ids) = 0;

		void reloadEntity(const String& id, const EntityData* data = nullptr);
		virtual void reloadEntities(gsl::span<const String> ids, gsl::span<const EntityData*> datas) = 0;
		virtual EntityTree getEntityTree() const = 0;
		virtual std::pair<String, size_t> reparentEntity(const String& entityId, const String& newParentId, size_t childIndex) = 0;
		virtual std::pair<String, size_t> getEntityParenting(const String& entityId) = 0;
        virtual bool isSingleRoot() = 0;
		virtual Vector<const EntityData*> getEntityDataStack(const String& entityId) = 0;
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

		struct SelectResult {
			bool hasHighlight = false;
			bool allowEntitySpriteSelection = true;
			bool blockRightClick = false;
		};

		using GizmoFactory = std::function<std::unique_ptr<SceneEditorGizmo>(SnapRules snapRules, const String& componentName, const String& fieldName)>;
		
		virtual ~ISceneEditorGizmoCollection() = default;

        virtual SelectResult update(Time time, const Camera& camera, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState, SceneEditorOutputState& outputState) = 0;
        virtual void draw(Painter& painter, const ISceneEditor& sceneEditor) = 0;
        virtual void setSelectedEntities(Vector<EntityRef> entities, Vector<EntityData*> entityDatas) = 0;
		virtual void refreshEntity() = 0;
        virtual std::shared_ptr<UIWidget> setTool(const String& tool, const String& componentName, const String& fieldName) = 0;
		virtual void deselect() = 0;
		virtual void addTool(const Tool& tool, GizmoFactory gizmoFactory) = 0;
		virtual void generateList(UIList& list) = 0;
		virtual ISceneEditorWindow& getSceneEditorWindow() = 0;
		virtual bool canBoxSelectEntities() = 0;
		virtual bool canSelectEntities() = 0;
	};

	class IProject;
	
	class ISceneEditorWindow {
	public:
		virtual ~ISceneEditorWindow() = default;

		virtual void markModified() = 0;
		
		virtual void onEntityModified(const String& id, const EntityData& prevData, const EntityData& newData) = 0;
		virtual void onEntitiesModified(gsl::span<const String> ids, gsl::span<const EntityData*> prevDatas, gsl::span<const EntityData*> newData) = 0;
		virtual void onComponentRemoved(const String& name) = 0;

		virtual void removeEntities(gsl::span<const String> entityIds) = 0;
		
		virtual const std::shared_ptr<ISceneData>& getSceneData() const = 0;

		virtual void addComponentToCurrentEntity(const String& componentName) = 0;
		virtual void setHighlightedComponents(Vector<String> componentNames) = 0;
		virtual const IEntityEditorFactory& getEntityEditorFactory() const = 0;

		virtual std::shared_ptr<ScriptNodeTypeCollection> getScriptNodeTypes() = 0;

		virtual const ConfigNode& getSetting(EditorSettingType type, std::string_view id) const = 0;
		virtual void setSetting(EditorSettingType type, std::string_view id, ConfigNode data) = 0;

		virtual float getProjectDefaultZoom() const = 0;

		virtual std::shared_ptr<EntityFactory> getEntityFactory() const = 0;
		virtual UIRoot& getUIRoot() = 0;

		virtual Path getPrimaryInputFile(AssetType type, const String& assetId, bool absolute) const = 0;

		virtual String getCurrentAssetId() const = 0;
		virtual bool isScene() const = 0;
		virtual String getSceneNameForComments() const = 0;
		virtual Vector2f getWorldOffset() const = 0;
		virtual Vector2f getWorldCameraPosition() const = 0;
		virtual void setCameraPosition(Vector2f pos) = 0;

		virtual Resources& getGameResources() const = 0;

		virtual Vector<const EntityData*> getEntityDataStack(const UUID& instanceUUID) const = 0;

		virtual IProject& getProject() const = 0;
	};

	class Game;

	class IProject {
    public:		
        virtual ~IProject() = default;
		virtual Path getAssetsSrcPath() const = 0;
		virtual bool writeAssetToDisk(const Path& filePath, gsl::span<const gsl::byte> data) = 0;
		virtual bool writeAssetToDisk(const Path& filePath, const Bytes& data) = 0;
		virtual bool writeAssetToDisk(const Path& filePath, std::string_view str) = 0;
		virtual void setAssetSaveNotification(bool enabled) = 0;
		virtual Game* getGameInstance() const = 0;
	};

	class IProjectWindow {
	public:
		virtual void addTask(std::unique_ptr<Task> task) = 0;
		virtual const ConfigNode& getSetting(EditorSettingType type, std::string_view id) const = 0;
        virtual void setSetting(EditorSettingType type, std::string_view id, ConfigNode data) = 0;
		virtual void buildGame() = 0;
		virtual void updateEditor() = 0;
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

        virtual Vector<ToolData> makeTools(const MakeToolArgs& args) = 0;
    };
}
