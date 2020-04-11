#pragma once
#include "halley/core/game/scene_editor_interface.h"
#include "halley/core/graphics/camera.h"
#include "../entity.h"

namespace Halley {
	class EntityRef;
	class World;
	class Painter;
	class SceneEditorGizmoCollection;
	
    class SceneEditor : public ISceneEditor {
    public:
		SceneEditor();
    	virtual ~SceneEditor();
    	
		void init(SceneEditorContext& context) override;
		void update(Time t, SceneEditorInputState inputState) override;
		void render(RenderContext& rc) override;

		World& getWorld() override;
		void spawnPending() override;

		EntityId getCameraId() override;
		void dragCamera(Vector2f amount) override;
		void changeZoom(int amount, Vector2f cursorPosRelToCamera) override;

		void setSelectedEntity(const UUID& id) override;
		void showEntity(const UUID& id) override;
		void setTool(SceneEditorTool tool) override;
    	
		std::vector<std::unique_ptr<IComponentEditorFieldFactory>> getComponentEditorFieldFactories() override;

		static Rect4f getSpriteTreeBounds(const EntityRef& e);
		static std::optional<Rect4f> getSpriteBounds(const EntityRef& e);

    protected:
		virtual void createServices(World& world, SceneEditorContext& context);
		virtual void createEntities(World& world, SceneEditorContext& context);

		virtual String getSceneEditorStageName();

    	virtual EntityId createCamera();

    private:
		std::unique_ptr<World> world;

    	EntityId cameraEntityId;
    	Camera camera;
    	
		std::optional<EntityRef> selectedEntity;
    	std::unique_ptr<SceneEditorGizmoCollection> gizmoCollection;

		std::unique_ptr<World> createWorld(SceneEditorContext& context);

    	void moveCameraTo2D(Vector2f pos);
		static void doGetSpriteTreeBounds(const EntityRef& e, std::optional<Rect4f>& rect);
    	Vector2f roundPosition(Vector2f pos) const;
		Vector2f roundPosition(Vector2f pos, float zoom) const;
	};
}
