#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/maths/uuid.h"

namespace Halley {
    class EntityData {
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
        
    	static EntityData makeDelta(const EntityData& from, const EntityData& to);
    	void applyDelta(const EntityData& delta);

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
    	uint8_t fieldPresent = 0;

    	void addComponent(String key, ConfigNode data);
    	void parseUUID(UUID& dst, const ConfigNode& node);

    	uint8_t getFieldBit(FieldId id) const;
    	void setFieldPresent(FieldId id, bool present);
    	bool isFieldPresent(FieldId id) const;
    };
}
