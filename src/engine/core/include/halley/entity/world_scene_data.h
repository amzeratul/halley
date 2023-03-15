#pragma once

#include "world.h"
#include "halley/game/scene_editor_interface.h"

namespace Halley {
    class WorldSceneData : public ISceneData {
    public:
    	explicit WorldSceneData(World& world);

    	std::optional<EntityNodeData> tryGetWriteableEntityNodeData(const String& id) override;
        std::pair<Vector<UUID>, Vector<EntityData*>> getWriteableEntityDatas(gsl::span<const UUID> ids) override;
        std::optional<ConstEntityNodeData> tryGetEntityNodeData(const String& id) override;
        void reloadEntities(gsl::span<const String> ids, gsl::span<const EntityData*> datas) override;
        EntityTree getEntityTree() const override;
        std::pair<String, size_t> reparentEntity(const String& entityId, const String& newParentId, size_t childIndex) override;
        std::pair<String, size_t> getEntityParenting(const String& entityId) override;
        bool isSingleRoot() override;
    	Vector<const EntityData*> getEntityDataStack(const String& entityId) override;
    	
    private:
    	World& world;
    };
}
