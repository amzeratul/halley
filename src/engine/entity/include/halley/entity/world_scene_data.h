#pragma once

#include "world.h"
#include "halley/core/game/scene_editor_interface.h"

namespace Halley {
    class WorldSceneData : public ISceneData {
    public:
    	explicit WorldSceneData(World& world);

    	EntityNodeData getWriteableEntityNodeData(const String& id) override;
        ConstEntityNodeData getEntityNodeData(const String& id) override;
        void reloadEntity(const String& id) override;
        EntityTree getEntityTree() const override;
        std::pair<String, size_t> reparentEntity(const String& entityId, const String& newParentId, size_t childIndex) override;
        bool isSingleRoot() override;
    	
    private:
    	World& world;
    };
}
