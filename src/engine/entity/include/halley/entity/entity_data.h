#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/maths/uuid.h"
#include <set>

namespace Halley {
	class EntityData;
    class EntityDataDelta;

	class IEntityData {
	public:
		virtual ~IEntityData() = default;

		virtual bool isDelta() const = 0;
		virtual const EntityData& asEntityData() const = 0;
		virtual const EntityDataDelta& asEntityDataDelta() const = 0;
	};

	struct EntityChangeOperation {
		std::unique_ptr<IEntityData> data;
		String entityId;
		String parent;
		int childIndex = -1;

		EntityChangeOperation clone() const;
        bool operator==(const EntityChangeOperation& other) const;
        bool operator!=(const EntityChangeOperation& other) const;
	};
	
    class EntityData : public IEntityData {
    	friend class EntityDataDelta;
    	
    public:
        enum class Flag : uint8_t {
	        NotSelectable = 1
        };

    	EntityData();
    	
        explicit EntityData(UUID instanceUUID);
    	EntityData(const ConfigNode& data, bool isPrefab);
        EntityData(const EntityData& other) = default;
    	EntityData(EntityData&& other) noexcept = default;
    	EntityData& operator=(const EntityData& other) = delete;
    	EntityData& operator=(EntityData&& other) noexcept = default;
        explicit EntityData(const EntityDataDelta& delta);

    	ConfigNode toConfigNode(bool allowPrefabUUID) const;
        String toYAML() const;

    	void serialize(Serializer& s) const;
    	void deserialize(Deserializer& s);

    	const String& getName() const { return name; }
    	const String& getPrefab() const { return prefab; }
    	const String& getIcon() const { return icon; }
        uint8_t getFlags() const { return flags; }
        bool getFlag(Flag flag) const;
    	const UUID& getInstanceUUID() const { return instanceUUID; }
    	const UUID& getPrefabUUID() const { return prefabUUID; }
    	const UUID& getParentUUID() const { return parentUUID; }
    	
    	const Vector<EntityData>& getChildren() const { return children; }
    	Vector<EntityData>& getChildren() { return children; }
    	const Vector<std::pair<String, ConfigNode>>& getComponents() const { return components; }
    	Vector<std::pair<String, ConfigNode>>& getComponents() { return components; }
        bool hasComponent(const String& componentName) const;

  	    const EntityData* tryGetPrefabUUID(const UUID& uuid) const;
        const EntityData* tryGetInstanceUUID(const UUID& uuid) const;
    	EntityData* tryGetInstanceUUID(const UUID& uuid);

    	void setName(String name);
    	void setPrefab(String prefab);
    	void setIcon(String icon);
        void setFlag(Flag flag, bool value);
        void randomiseInstanceUUIDs();
    	void setInstanceUUID(UUID instanceUUID);
    	void setPrefabUUID(UUID prefabUUID);
        void setParentUUID(UUID parentUUID);
	   	void setChildren(Vector<EntityData> children);
    	void setComponents(Vector<std::pair<String, ConfigNode>> components);

    	void applyDelta(const EntityDataDelta& delta);
        static EntityData applyDelta(EntityData src, const EntityDataDelta& delta);

    	bool matchesUUID(const UUID& uuid) const;
    	bool matchesUUID(const EntityData& other) const;

        void updateComponent(const String& id, const ConfigNode& data);
        void updateChild(const EntityData& instanceChildData);

    	void instantiateWith(const EntityData& instance);
	    EntityData instantiateWithAsCopy(const EntityData& instance) const;
    	
        bool isDelta() const override;
        const EntityData& asEntityData() const override;
        const EntityDataDelta& asEntityDataDelta() const override;

    	void setSceneRoot(bool root);
    	bool isSceneRoot() const;

        std::optional<size_t> getChildIndex(const UUID& uuid) const;

    private:

    	String name;
    	String prefab;
    	String icon;
        uint8_t flags = 0;
    	UUID instanceUUID;
    	UUID prefabUUID;
    	UUID parentUUID;
    	Vector<EntityData> children;
    	Vector<std::pair<String, ConfigNode>> components;
    	bool sceneRoot = false;

    	void addComponent(String key, ConfigNode data);
    	void parseUUID(UUID& dst, const ConfigNode& node);
    	void generateChildUUID(const UUID& root);
    	void instantiateData(const EntityData& instance);
	};
}
