#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/maths/uuid.h"
#include <set>

namespace Halley {
    class EntityData;
	
	class EntityDataDelta {
		friend class EntityData;
		
	public:
        class Options {
        public:
            Options();
        	
	        bool preserveOrder;
        	std::set<String> ignoreComponents;
        };
		
        EntityDataDelta();
		explicit EntityDataDelta(const EntityData& to, const Options& options = Options());
		EntityDataDelta(const EntityData& from, const EntityData& to, const Options& options = Options());

		bool hasChange() const;

		void serialize(Serializer& s) const;
    	void deserialize(Deserializer& s);

		const std::optional<String>& getPrefab() const { return prefab; }
		const std::optional<UUID>& getPrefabUUID() const { return prefabUUID; }
		void setPrefabUUID(const UUID& uuid);

		bool isSimpleDelta() const;

	private:
    	std::optional<String> name;
    	std::optional<String> prefab;
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
        	ChildrenOrder
        };

    	static uint16_t getFieldBit(FieldId id);
    	static void setFieldPresent(uint16_t& value, FieldId id, bool present);
    	static bool isFieldPresent(uint16_t value, FieldId id);

		uint16_t getFieldsPresent() const;
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
