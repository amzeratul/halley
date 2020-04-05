#pragma once
#include "entity_factory.h"
#include "halley/core/game/scene_editor_interface.h"

namespace Halley {
    class PrefabSceneData : public ISceneData {
    public:
    	PrefabSceneData(Prefab& prefab, std::shared_ptr<EntityFactory> factory, EntityRef entity);

        ConfigNode getEntityData(const String& id) override;
        void reloadEntity(const String& id, const ConfigNode& data) override;
        EntityTree getEntityTree() const override;
    	
    private:
    	Prefab& prefab;
        std::shared_ptr<EntityFactory> factory;
        EntityRef entity;

    	ConfigNode* findEntity(ConfigNode& node, const String& id) const;
    	void fillEntityTree(const ConfigNode& node, EntityTree& tree) const;
    };
}
