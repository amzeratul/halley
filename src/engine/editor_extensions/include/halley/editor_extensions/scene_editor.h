#pragma once

#include "halley/core/game/scene_editor_interface.h"
#include "halley/core/graphics/camera.h"
#include "halley/entity/entity.h"
#include "halley/core/graphics/text/text_renderer.h"

namespace Halley {
	struct SceneEditorOutputState;
	class EntityRef;
	class World;
	class Painter;
	class SceneEditorGizmoCollection;
	
    class SceneEditor : public ISceneEditor {
    public:
		SceneEditor();
    	virtual ~SceneEditor();

		void init(SceneEditorContext& context) final override;
		void update(Time t, SceneEditorInputState inputState, SceneEditorOutputState& outputState) override;
		void render(RenderContext& rc) override;

		bool isReadyToCreateWorld() const final override;
		void createWorld(std::shared_ptr<const UIColourScheme> colourScheme) final override;

		World& getWorld() const override;
		Resources& getResources() const override;
		void spawnPending() override;

    	const std::vector<EntityId>& getCameraIds() const override;
		void dragCamera(Vector2f amount) override;
		void moveCamera(Vector2f pos) override;
    	bool loadCameraPos() override;
		void changeZoom(int amount, Vector2f cursorPosRelToCamera) override;

    	void setupTools(UIList& toolList, ISceneEditorGizmoCollection& gizmoCollection) override;

    	void setSelectedEntity(const UUID& id, EntityData& entityData) override;
    	void setEntityHighlightedOnList(const UUID& id) override;

		void onEntityAdded(const UUID& id, const EntityData& entityData) final override;
		void onEntityRemoved(const UUID& id) final override;
		void onEntityMoved(const UUID& id, const EntityData& entityData) final override;
		void onEntityModified(const UUID& id, const EntityData& entityData) final override;
		virtual void onEntityAdded(EntityRef entity, const EntityData& entityData);
		virtual void onEntityRemoved(EntityRef entity);
		virtual void onEntityMoved(EntityRef entity, const EntityData& entityData);
		virtual void onEntityModified(EntityRef entity, const EntityData& entityData);

		void showEntity(const UUID& id) override;
		void onToolSet(String& tool, String& componentName, String& fieldName) override;
    	
		std::vector<std::unique_ptr<IComponentEditorFieldFactory>> getComponentEditorFieldFactories() override;
		std::shared_ptr<UIWidget> makeCustomUI() override;

		void onSceneLoaded(Prefab& scene) override;
    	void onSceneSaved() override;
    	
		Rect4f getSpriteTreeBounds(const EntityRef& e) const override;
		std::optional<Rect4f> getSpriteBounds(const EntityRef& e) const;
		virtual bool isSpriteVisibleOnCamera(const Sprite& sprite, OptionalLite<int> mask) const;

    	std::optional<Vector2f> getMousePos() const override;
		Vector2f getCameraPos() const override;
    	
    	void setupConsoleCommands(UIDebugConsoleController& controller, ISceneEditorWindow& sceneEditor) override;

		void refreshAssets() override;

		std::shared_ptr<ScriptNodeTypeCollection> getScriptNodeTypes() override;

		std::vector<UIPopupMenuItem> getSceneContextMenu(const Vector2f& mousePos) const override;
    	void onSceneContextMenuSelection(const String& id) override;
    	void onSceneContextMenuHighlight(const String& id) override;

		std::vector<AssetCategoryFilter> getPrefabCategoryFilters() const override;
    	Future<AssetPreviewData> getAssetPreviewData(AssetType assetType, const String& id) override;

    protected:
    	Camera camera;
		IEditorInterface* editorInterface;

		virtual void onInit(std::shared_ptr<const UIColourScheme> colourScheme);
    	
		virtual void createServices(World& world);
		virtual void createEntities(World& world);

		virtual String getSceneEditorStageName() const;
    	const HalleyAPI& getAPI() const;
    	Resources& getGameResources() const;
    	Resources& getEditorResources() const;

    	virtual void drawOverlay(Painter& painter, Rect4f view);

    	virtual std::vector<EntityId> createCamera();

    	virtual void onEntitySelected(std::optional<EntityRef> entity);
    	virtual void setEntityFocus(std::vector<EntityId> entityIds);
		void cycleHighlight(int delta) override;

		virtual std::optional<Vector2f> getWorldOffset() const;
		std::unique_ptr<World> doCreateWorld(const String& stageName) const;

    	virtual std::vector<EntityRef> getEntitiesAt(Vector2f point) const;
    	EntityRef getRootEntityAt(Vector2f point) const;
    	std::vector<EntityRef> getRootEntitiesAt(Vector2f point) const;
       	virtual float getSpriteDepth(EntityRef& e, Vector2f point) const;

	private:
		const HalleyAPI* api = nullptr;
		Resources* resources = nullptr;
		Resources* editorResources = nullptr;

    	std::unique_ptr<World> world;

		std::vector<EntityId> cameraEntityIds;
    	
		std::optional<EntityRef> selectedEntity;
    	std::optional<EntityRef> forceFocusEntity;
    	EntityRef focusedEntity;
		EntityRef entityHighlightedOnList;
    	bool focusEntityEnabled = false;
    	mutable int highlightDelta = 0;
    	ISceneEditorGizmoCollection* gizmoCollection = nullptr;

    	TextRenderer coordinateInfo;
		std::optional<Vector2f> mousePos;
    	std::optional<Vector2f> holdMouseStart;
    	std::optional<Rect4f> selBox;

    	void moveCameraTo2D(Vector2f pos);
		void doGetSpriteTreeBounds(const EntityRef& e, std::optional<Rect4f>& rect) const;
    	Vector2f roundPosition(Vector2f pos) const;
		Vector2f roundPosition(Vector2f pos, float zoom) const;
    	EntityRef getEntity(const UUID& uuid) const;

    	bool isPointInSprite(EntityRef& e, Vector2f point) const;
    	void onClick(const SceneEditorInputState& input, SceneEditorOutputState& output);

    	EntityRef getEntityToFocus();
		void updateEntityFocused();
		void addEntityIdToList(std::vector<EntityId>& dst, EntityRef entity);

    	void saveCameraPos();
	};
}
