#pragma once

#include "world.h"
#include "halley/core/game/scene_editor_interface.h"

namespace Halley {
    class WorldSceneData : public ISceneData {
    public:
    	explicit WorldSceneData(World& world);

        ConfigNode getEntityData(const String& id) override;
        void reloadEntity(const String& id, const ConfigNode& data) override;
        EntityTree getEntityTree() const override;
    	
    private:
    	World& world;
    };
}
