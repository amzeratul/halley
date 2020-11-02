#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/maths/uuid.h"
#include <set>

namespace Halley {
    class EntityDataDelta;
	
    class EntityData {
    	friend class EntityDataDelta;
    	
    public:
    	EntityData();
        explicit EntityData(UUID instanceUUID);
    	explicit EntityData(const ConfigNode& data);
        EntityData(const EntityData& other) = default;
    	EntityData(EntityData&& other) = default;
    	EntityData& operator=(const EntityData& other) = delete;
    	EntityData& operator=(EntityData&& other) = default;

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

  	    const EntityData* tryGetPrefabUUID(const UUID& uuid) const;

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

        void updateComponent(const String& id, const ConfigNode& data);
        void updateChild(const EntityData& instanceChildData);

    	void instantiateWith(const EntityData& instance);
	    EntityData instantiateWithAsCopy(const EntityData& instance) const;

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
    	void generateChildUUID(const UUID& root);
    	void instantiateData(const EntityData& instance);
	};

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
