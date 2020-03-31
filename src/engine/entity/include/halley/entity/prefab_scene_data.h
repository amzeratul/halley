#pragma once
#include "halley/core/game/scene_editor_interface.h"

namespace Halley {
    class PrefabSceneData : public ISceneData {
    public:
    	PrefabSceneData(Prefab& prefab);

        ConfigNode getEntityData(const String& id) override;
    	
    private:
    	Prefab& prefab;

    	Maybe<ConfigNode> findEntity(ConfigNode& node, const String& id);
    };
}
