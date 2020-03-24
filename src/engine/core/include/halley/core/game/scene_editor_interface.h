#pragma once

#include "halley/time/halleytime.h"

namespace Halley {
    class RenderContext;
    class World;
	
    class SceneEditorInterface {
    public:
        virtual ~SceneEditorInterface() = default;

        virtual void update(Time t) = 0;
        virtual void render(RenderContext& rc) = 0;
        virtual World& getWorld() = 0;
    };
}
