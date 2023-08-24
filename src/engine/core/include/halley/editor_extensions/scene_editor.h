#pragma once

#include "asset_preview_generator.h"
#include "halley/game/scene_editor_interface.h"
#include "halley/graphics/camera.h"
#include "halley/entity/entity.h"
#include "halley/graphics/text/text_renderer.h"

namespace Halley {
	class UIDebugConsoleCommands;
	class BaseFrameData;
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
		void update(Time t, SceneEditorInputState inputState, SceneEditorOutputState& outputState) final override;
		void render(RenderContext& rc) final override;

    	virtual void preUpdate(Time t);
    	virtual void postUpdate(Time t);
		virtual void preRender(RenderContext& rc);
		virtual void postRender(RenderContext& rc);

    	bool isReadyToCreateWorld() const final override;
		void createWorld(std::shared_ptr<const UIColourScheme> colourScheme) final override;

		World& getWorld() const override;
		Resources& getResources() const override;
		void spawnPending() override;

    	const Vector<EntityId>& getCameraIds() const override;
		void dragCamera(Vector2f amount) override;
		void moveCamera(Vector2f pos) override;
    	bool loadCameraPos() override;
        void adjustView(int zoomChange, bool zoomToFit, bool centre) override;
		void changeZoom(int amount, Vector2f cursorPosRelToCamera) override;

    	void setupTools(UIList& toolList, ISceneEditorGizmoCollection& gizmoCollection) override;

    	void setSelectedEntities(Vector<UUID> uuids, Vector<EntityData*> datas) override;
    	void setEntityHighlightedOnList(const UUID& id, bool forceShow) override;

		void showEntity(const UUID& id) override;
		void onToolSet(String& tool, String& componentName, String& fieldName) override;
    	
		std::shared_ptr<UIWidget> makeCustomUI() override;

		void onSceneLoaded(Prefab& scene) override;
    	void onSceneSaved() override;

    	std::optional<Vector2f> getMousePos() const override;
		Vector2f getCameraPos() const override;
		std::optional<Vector2f> getWorldOffset() const override;
		float getZoom() const override;
    	
    	void setupConsoleCommands(UIDebugConsoleCommands& commands, ISceneEditorWindow& sceneEditor) override;

		void refreshAssets() override;

		Vector<UIPopupMenuItem> getSceneContextMenu(const Vector2f& mousePos) const override;
    	void onSceneContextMenuSelection(const String& id) override;
    	void onSceneContextMenuHighlight(const String& id) override;

    	void setAssetPreviewGenerator(AssetPreviewGenerator& generator) override;

		Transform2DComponent* getTransform(const String& entityId) override;

		void initializeEntityValidator(EntityValidator& validator) override;

		bool shouldDrawOutline(const Sprite& sprite) const override;

    	String getSceneNameForComments(AssetType assetType, const String& assetId) const override;

    protected:
		enum class EntityAtPositionSelectMode {
			All,
			Sprite,
			NonSprite
		};

    	Camera camera;
		Vector2i viewPort;
		IEditorInterface* editorInterface;

		virtual void onInit(std::shared_ptr<const UIColourScheme> colourScheme);
    	
		virtual void createServices(World& world, std::shared_ptr<const UIColourScheme> colourScheme);
		virtual void createEntities(World& world);

		virtual String getSceneEditorStageName() const;
    	const HalleyAPI& getAPI() const;
    	Resources& getGameResources() const;
    	Resources& getEditorResources() const;

    	virtual void drawOverlay(Painter& painter, Rect4f view);

    	virtual Vector<EntityId> createCamera();

    	virtual void onEntitiesSelected(Vector<EntityId> entityIds);
    	virtual void setEntityFocus(Vector<EntityId> entityIds);
		void cycleHighlight(int delta) override;

		std::unique_ptr<World> doCreateWorld(const String& stageName) const;

    	virtual Vector<EntityRef> getEntitiesAt(Rect4f area, bool allowUnselectable, EntityAtPositionSelectMode mode) const;
    	EntityRef getRootEntityAt(Vector2f point, bool allowUnselectable, EntityAtPositionSelectMode mode) const;
		Vector<EntityRef> getRootEntitiesAt(Vector2f point, bool allowUnselectable, EntityAtPositionSelectMode mode) const;
    	Vector<EntityRef> getRootEntitiesAt(Rect4f area, bool allowUnselectable, EntityAtPositionSelectMode mode) const;
       	virtual float getSpriteDepth(EntityRef& e, Rect4f area) const;

		virtual std::unique_ptr<BaseFrameData> makeFrameData();

	private:
		struct CameraAnimation {
			Vector2f p0;
			Vector2f p1;
			float z0;
			float z1;
			float t = 0;
		};

		struct CameraPanAnimation {
			Vector<std::pair<Vector2f, Time>> deltas;
			std::optional<Vector2f> inertiaVel;
			bool updatedLastFrame = false;
			void stop();
		};

		const HalleyAPI* api = nullptr;
		Resources* resources = nullptr;
		Resources* editorResources = nullptr;

    	std::unique_ptr<World> world;

		Vector<EntityId> cameraEntityIds;
		std::optional<CameraAnimation> cameraAnimation;
		CameraPanAnimation cameraPanAnimation;
		Time lastStepTime = 0;
    	
		Vector<EntityId> selectedEntityIds;
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
		Vector<UUID> selBoxStartSelectedEntities;

		AssetPreviewGenerator* assetPreviewGenerator = nullptr;

		std::unique_ptr<BaseFrameData> curFrameData;
		mutable uint8_t curFrameDataDepth = 0;

    	void moveCameraTo2D(Vector2f pos);
    	Vector2f roundPosition(Vector2f pos) const;
		Vector2f roundPosition(Vector2f pos, float zoom) const;
		Vector2f roundCameraPosition(Vector2f pos, float zoom) const;
    	EntityRef getEntity(const UUID& uuid) const;

    	bool doesAreaOverlapSprite(EntityRef& e, Rect4f area) const;
    	void onClick(const SceneEditorInputState& input, SceneEditorOutputState& output, bool canSelectSprite, bool canSelectEntities);
		void onStartSelectionBox();
		void onSelectionBox(const SceneEditorInputState& input, SceneEditorOutputState& output);

    	EntityRef getEntityToFocus();
		void updateEntityFocused();
		void addEntityIdToList(Vector<EntityId>& dst, EntityRef entity);

		void updateCameraPos(Time t);
    	void saveCameraPos();

		void pushThreadFrameData() const;
		void popThreadFrameData() const;
	};
}
