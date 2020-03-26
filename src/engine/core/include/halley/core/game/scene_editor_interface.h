#pragma once

#include "halley/entity/entity_id.h"
#include "halley/time/halleytime.h"

namespace Halley {
    class RenderContext;
    class World;
    class HalleyAPI;
    class Resources;

    class SceneEditorContext {
    public:
        const HalleyAPI* api;
        Resources* resources;
    };
	
    class SceneEditorInterface {
    public:
        virtual ~SceneEditorInterface() = default;

        virtual void init(SceneEditorContext& context) = 0;
        virtual void update(Time t) = 0;
        virtual void render(RenderContext& rc) = 0;
    	
        virtual World& getWorld() = 0;
        virtual void spawnPending() = 0;
        virtual EntityId getCameraId() = 0;
    };
}
