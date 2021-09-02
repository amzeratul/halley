#pragma once
#include "entity_factory.h"
#include "halley/core/game/scene_editor_interface.h"

namespace Halley {
    class PrefabSceneData final : public ISceneData {
    public:
    	PrefabSceneData(Prefab& prefab, std::shared_ptr<EntityFactory> factory, World& world, Resources& gameResources);

        EntityNodeData getWriteableEntityNodeData(const String& id) override;
        ConstEntityNodeData getEntityNodeData(const String& id) override;
        void reloadEntity(const String& id) override;
        EntityTree getEntityTree() const override;
        std::pair<String, size_t> reparentEntity(const String& entityId, const String& newParentId, size_t childIndex) override;
        bool isSingleRoot() override;
    	
    private:
    	Prefab& prefab;
        std::shared_ptr<EntityFactory> factory;
        World& world;
    	Resources& gameResources;

    	struct EntityAndParent {
    		EntityData* entity = nullptr;
    		EntityData* parent = nullptr;
    		size_t childIdx = 0;
    	};

    	void reloadEntity(const String& id, EntityData* data);
        void fillEntityTree(const EntityData& node, EntityTree& tree) const;
    	void fillPrefabChildren(const EntityData& node, std::vector<String>& dst) const;

        EntityData& findEntity(const String& id);

    	static EntityData* findEntity(gsl::span<EntityData> node, const String& id);
    	static EntityAndParent findEntityAndParent(gsl::span<EntityData> node, EntityData* previous, size_t idx, const String& id);
        static EntityAndParent findEntityAndParent(EntityData& node, EntityData* previous, size_t idx, const String& id);

        static void addChild(EntityData& parent, int index, EntityData child);
        static EntityData removeChild(EntityData& parent, const String& childId);
        static void moveChild(EntityData& parent, const String& childId, int targetIndex);

    	static void makeTransformRelative(EntityRef entity, std::optional<EntityRef> newParent, EntityData& entityData);
    	static void makeTransformRelative2D(EntityRef entity, std::optional<EntityRef> newParent, EntityData& entityData);
    };
}
