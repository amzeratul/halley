#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/maths/uuid.h"

namespace Halley {
    class EntityDataDelta;
	
    class EntityData {
    	friend class EntityDataDelta;
    	
    public:
    	EntityData();
        explicit EntityData(UUID instanceUUID);
    	explicit EntityData(const ConfigNode& data);

    	ConfigNode toConfigNode() const;

    	void serialize(Serializer& s) const;
    	void deserialize(Deserializer& s);

    	const String& getName() const { return name; }
    	const String& getPrefab() const { return prefab; }
    	const UUID& getInstanceUUID() const { return instanceUUID; }
    	const UUID& getPrefabUUID() const { return prefabUUID; }
    	const UUID& getParentUUID() const { return parentUUID; }
    	
    	const std::vector<EntityData>& getChildren() const { return children; }
    	std::vector<EntityData>& getChildren() { return children; }
    	const std::vector<std::pair<String, ConfigNode>>& getComponents() const { return components; }
    	std::vector<std::pair<String, ConfigNode>>& getComponents() { return components; }

    	void setName(String name);
    	void setPrefab(String prefab);
    	void setInstanceUUID(UUID instanceUUID);
    	void setPrefabUUID(UUID prefabUUID);
        void setParentUUID(UUID parentUUID);
	   	void setChildren(std::vector<EntityData> children);
    	void setComponents(std::vector<std::pair<String, ConfigNode>> components);

    	void applyDelta(const EntityDataDelta& delta);
        static EntityData applyDelta(EntityData src, const EntityDataDelta& delta);

    	bool isSameEntity(const EntityData& other) const;

    private:    	
    	String name;
    	String prefab;
    	UUID instanceUUID;
    	UUID prefabUUID;
    	UUID parentUUID;
    	std::vector<EntityData> children;
    	std::vector<std::pair<String, ConfigNode>> components;

    	void addComponent(String key, ConfigNode data);
    	void parseUUID(UUID& dst, const ConfigNode& node);
	};

	class EntityDataDelta {
		friend class EntityData;
		
	public:
        class Options {
        public:
	        bool preserveOrder = false;
        };
		
        EntityDataDelta();
		explicit EntityDataDelta(const EntityData& to, Options options = {});
		EntityDataDelta(const EntityData& from, const EntityData& to, Options options = {});

		bool hasChange() const;

		void serialize(Serializer& s) const;
    	void deserialize(Deserializer& s);

		const std::optional<String>& getPrefab() const { return prefab; }

	private:
    	std::optional<String> name;
    	std::optional<String> prefab;
    	std::optional<UUID> instanceUUID;
    	std::optional<UUID> prefabUUID;
		std::optional<UUID> parentUUID;
		
		std::vector<std::pair<String, ConfigNode>> componentsChanged; // Add/modified
		std::vector<String> componentsRemoved; // Removed
		std::vector<String> componentOrder;

		std::vector<std::pair<UUID, EntityDataDelta>> childrenChanged; // Add/modified
		std::vector<UUID> childrenRemoved; // Removed
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
