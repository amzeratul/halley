#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/maths/uuid.h"

namespace Halley {
    class EntityDataDelta;
	
    class EntityData {
    	friend class EntityDataDelta;
    	
    public:
    	EntityData();
    	EntityData(const ConfigNode& data);

    	ConfigNode toConfigNode() const;

    	void serialize(Serializer& s) const;
    	void deserialize(Deserializer& s);

    	const String& getName() const { return name; }
    	const String& getPrefab() const { return prefab; }
    	const UUID& getInstanceUUID() const { return instanceUUID; }
    	const UUID& getPrefabUUID() const { return prefabUUID; }
    	
    	const std::vector<EntityData>& getChildren() const { return children; }
    	std::vector<EntityData>& getChildren() { return children; }
    	const std::vector<std::pair<String, ConfigNode>>& getComponents() const { return components; }
    	std::vector<std::pair<String, ConfigNode>>& getComponents() { return components; }

    	void setName(String name);
    	void setPrefab(String prefab);
    	void setInstanceUUID(UUID instanceUUID);
    	void setPrefabUUID(UUID prefabUUID);
    	void setChildren(std::vector<EntityData> children);
    	void setComponents(std::vector<std::pair<String, ConfigNode>> components);

    	void applyDelta(const EntityDataDelta& delta);
    	static EntityData applyDelta(EntityData src, const EntityDataDelta& delta);

    	bool isSameEntity(const EntityData& other) const;

    private:
        enum class FieldId {
	        Name,
        	Prefab,
        	InstanceUUID,
        	PrefabUUID,
        	Children,
        	Components
        };
    	
    	String name;
    	String prefab;
    	UUID instanceUUID;
    	UUID prefabUUID;
    	std::vector<EntityData> children;
    	std::vector<std::pair<String, ConfigNode>> components;

    	void addComponent(String key, ConfigNode data);
    	void parseUUID(UUID& dst, const ConfigNode& node);

    	static uint8_t getFieldBit(FieldId id);
    	static uint8_t setFieldPresent(uint8_t value, FieldId id, bool present);
    	static bool isFieldPresent(uint8_t value, FieldId id);
	};

	class EntityDataDelta {
		friend class EntityData;
		
	public:
        class Options {
        public:
	        bool preserveOrder = false;
        };
		
        EntityDataDelta();
		EntityDataDelta(const EntityData& from, const EntityData& to, Options options = {});

		bool hasChange() const;

		void serialize(Serializer& s) const;
    	void deserialize(Deserializer& s);

	private:
    	std::optional<String> name;
    	std::optional<String> prefab;
    	std::optional<UUID> instanceUUID;
    	UUID prefabUUID;
		
		std::vector<std::pair<String, ConfigNode>> componentsChanged; // Add/modified
		std::vector<String> componentsRemoved; // Removed
		std::vector<String> componentOrder;

		std::vector<EntityDataDelta> childrenChanged; // Add/modified
		std::vector<UUID> childrenRemoved; // Removed
		std::vector<UUID> childrenOrder;
	};
}
