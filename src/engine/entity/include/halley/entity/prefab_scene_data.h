#pragma once
#include "entity_factory.h"
#include "halley/core/game/scene_editor_interface.h"

namespace Halley {
    class PrefabSceneData : public ISceneData {
    public:
    	PrefabSceneData(Prefab& prefab, std::shared_ptr<EntityFactory> factory, World& world, Resources& gameResources);

        EntityData getEntityData(const String& id) override;
        void reloadEntity(const String& id) override;
        EntityTree getEntityTree() const override;
        void reparentEntity(const String& entityId, const String& newParentId, int childIndex) override;
    	
    private:
    	Prefab& prefab;
        std::shared_ptr<EntityFactory> factory;
        World& world;
    	Resources& gameResources;

    	void reloadEntity(const String& id, ConfigNode* data);
        void fillEntityTree(const ConfigNode& node, EntityTree& tree) const;

        static ConfigNode* findEntity(ConfigNode& node, const String& id);
        static std::pair<ConfigNode*, ConfigNode*> findEntityAndParent(ConfigNode& node, ConfigNode* previous, const String& id);

        static void addChild(ConfigNode& parent, int index, ConfigNode child);
        static ConfigNode removeChild(ConfigNode& parent, const String& childId);
        static void moveChild(ConfigNode& parent, const String& childId, int targetIndex);
    };
}
