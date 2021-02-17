#pragma once
#include "halley/core/editor_extensions/scene_editor_interface.h"
#include "halley/core/graphics/camera.h"
#include "../entity.h"
#include "halley/core/graphics/text/text_renderer.h"

namespace Halley {
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
		void spawnPending() override;

		const std::vector<EntityId>& getCameraIds() const override;
		void dragCamera(Vector2f amount) override;
		void changeZoom(int amount, Vector2f cursorPosRelToCamera) override;

		void setSelectedEntity(const UUID& id, EntityData& entityData) override;

		void onEntityAdded(const UUID& id, const EntityData& entityData) final override;
		void onEntityRemoved(const UUID& id) final override;
		void onEntityMoved(const UUID& id, const EntityData& entityData) final override;
		void onEntityModified(const UUID& id, const EntityData& entityData) final override;
		virtual void onEntityAdded(EntityRef entity, const EntityData& entityData);
		virtual void onEntityRemoved(EntityRef entity);
		virtual void onEntityMoved(EntityRef entity, const EntityData& entityData);
		virtual void onEntityModified(EntityRef entity, const EntityData& entityData);

		void showEntity(const UUID& id) override;
		ConfigNode onToolSet(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options) override;
    	
		std::vector<std::unique_ptr<IComponentEditorFieldFactory>> getComponentEditorFieldFactories() override;
		std::shared_ptr<UIWidget> makeCustomUI() override;

		void onSceneLoaded(Prefab& scene) override;
    	void onSceneSaved() override;
    	
		static Rect4f getSpriteTreeBounds(const EntityRef& e);
		static std::optional<Rect4f> getSpriteBounds(const EntityRef& e);

    	void setupConsoleCommands(UIDebugConsoleController& controller, ISceneEditorWindow& sceneEditor) override;

		void refreshAssets() override;
    
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

    	Vector2f getMousePos() const;
		std::unique_ptr<World> doCreateWorld(const String& stageName) const;

    	virtual EntityRef getEntityAt(Vector2f point) const;
       	virtual float getSpriteDepth(EntityRef& e, Vector2f point) const;

	private:
		const HalleyAPI* api = nullptr;
		Resources* resources = nullptr;
		Resources* editorResources = nullptr;

    	std::unique_ptr<World> world;

		std::vector<EntityId> cameraEntityIds;
    	
		std::optional<EntityRef> selectedEntity;
    	ISceneEditorGizmoCollection* gizmoCollection = nullptr;

    	TextRenderer coordinateInfo;
		Vector2f mousePos;
    	std::optional<Vector2f> holdMouseStart;
    	std::optional<Rect4f> selBox;

    	void moveCameraTo2D(Vector2f pos);
		static void doGetSpriteTreeBounds(const EntityRef& e, std::optional<Rect4f>& rect);
    	Vector2f roundPosition(Vector2f pos) const;
		Vector2f roundPosition(Vector2f pos, float zoom) const;
    	EntityRef getEntity(const UUID& uuid) const;

    	bool isPointInSprite(EntityRef& e, Vector2f point) const;
    	void onClick(const SceneEditorInputState& input, SceneEditorOutputState& output);
	};
}
