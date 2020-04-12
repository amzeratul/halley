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

        ConfigNode::SequenceType& findChildListFor(const String& id);
        static ConfigNode* doFindChildListFor(ConfigNode& node, const String& id);
    	
        static ConfigNode* findEntity(ConfigNode& node, const String& id);
        static std::pair<ConfigNode*, ConfigNode*> findEntityAndParent(ConfigNode& node, ConfigNode* previous, const String& id);

        static void addChild(ConfigNode::SequenceType& parent, int index, ConfigNode child);
        static ConfigNode removeChild(ConfigNode::SequenceType& parent, const String& childId);
        static void moveChild(ConfigNode::SequenceType& parent, const String& childId, int targetIndex);
    };
}
