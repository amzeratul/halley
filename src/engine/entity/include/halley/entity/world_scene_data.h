#pragma once

#include "world.h"
#include "halley/core/game/scene_editor_interface.h"

namespace Halley {
    class WorldSceneData : public ISceneData {
    public:
    	explicit WorldSceneData(World& world);

        EntityData getEntityData(const String& id) override;
        void reloadEntity(const String& id) override;
        EntityTree getEntityTree() const override;
        void reparentEntity(const String& entityId, const String& newParentId, int childIndex) override;
        bool isSingleRoot() override;
    	
    private:
    	World& world;
    };
}
