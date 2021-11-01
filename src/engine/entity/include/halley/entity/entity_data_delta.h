#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/maths/uuid.h"
#include <set>
#include "entity_data.h"

namespace Halley {	
	class EntityDataDelta : public IEntityData {
		friend class EntityData;
		
	public:
        class Options {
        public:
            Options();
        	
	        bool preserveOrder = false;
			bool shallow = false;
        	std::set<String> ignoreComponents;
        };
		
        EntityDataDelta();
		explicit EntityDataDelta(const EntityData& to, const Options& options = Options());
		EntityDataDelta(const EntityData& from, const EntityData& to, const Options& options = Options());

		bool hasChange() const;

		void serialize(Serializer& s) const;
    	void deserialize(Deserializer& s);

		const std::optional<String>& getName() const { return name; }
		const std::optional<String>& getPrefab() const { return prefab; }
		const std::optional<String>& getIcon() const { return icon; }
		const std::optional<UUID>& getPrefabUUID() const { return prefabUUID; }
		void setPrefabUUID(const UUID& uuid);
		
		const std::vector<std::pair<String, ConfigNode>>& getComponentsChanged() const { return componentsChanged; }
		const std::vector<String>& getComponentsRemoved() const { return componentsRemoved; }
		
		const std::vector<EntityData>& getChildrenAdded() const { return childrenAdded; }
		const std::vector<std::pair<UUID, EntityDataDelta>>& getChildrenChanged() const { return childrenChanged; }
		const std::vector<UUID>& getChildrenRemoved() const { return childrenRemoved; }

		bool isSimpleDelta() const;
    	
        bool isDelta() const override;
        const EntityData& asEntityData() const override;
        const EntityDataDelta& asEntityDataDelta() const override;

		bool modifiesTheSameAs(const EntityDataDelta& other) const;

	private:
    	std::optional<String> name;
    	std::optional<String> prefab;
		std::optional<String> icon;
    	std::optional<UUID> instanceUUID;
    	std::optional<UUID> prefabUUID;
		std::optional<UUID> parentUUID;

		std::vector<std::pair<String, ConfigNode>> componentsChanged;// Also includes components added
		std::vector<String> componentsRemoved;
		std::vector<String> componentOrder;

		std::vector<EntityData> childrenAdded;
		std::vector<std::pair<UUID, EntityDataDelta>> childrenChanged;
		std::vector<UUID> childrenRemoved;
		std::vector<UUID> childrenOrder;

        enum class FieldId {
        	RESERVED,
	        Name,
        	Prefab,
        	InstanceUUID,
        	PrefabUUID,
        	ParentUUID,
        	ComponentsChanged,
        	ComponentsRemoved,
        	ComponentsOrder,
        	ChildrenChanged,
        	ChildrenAdded,
        	ChildrenRemoved,
        	ChildrenOrder,
        	Icon
        };

    	static uint16_t getFieldBit(FieldId id);
    	static void setFieldPresent(uint16_t& value, FieldId id, bool present);
    	static bool isFieldPresent(uint16_t value, FieldId id);

		uint16_t getFieldsPresent() const;

		std::vector<std::pair<String, ConfigNode>> getComponentEmptyStructure() const;
	};

	class SceneDataDelta {
	public:
		void addEntity(UUID entityId, EntityDataDelta delta);
		const std::vector<std::pair<UUID, EntityDataDelta>>& getEntities() const;

		void serialize(Serializer& s) const;
    	void deserialize(Deserializer& s);

	private:
		std::vector<std::pair<UUID, EntityDataDelta>> entities;
	};
}
