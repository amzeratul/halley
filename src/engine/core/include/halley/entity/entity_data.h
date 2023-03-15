#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/maths/uuid.h"
#include <set>

namespace Halley {
	class EntityData;
    class EntityDataDelta;
    class EntityDataInstanced;

	class IEntityData {
	public:
        enum class Type {
	        Data,
            Delta,
            Instanced
        };

		virtual ~IEntityData() = default;

		virtual Type getType() const = 0;
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

    class IEntityConcreteData : public IEntityData {
    public:
        enum class Flag : uint8_t {
	        NotSelectable = 1,
            Disabled = 2,
            TreeViewCollapsed = 4
        };

    	virtual const String& getName() const = 0;
    	virtual const String& getPrefab() const = 0;
        virtual uint8_t getFlags() const = 0;
    	virtual bool getFlag(Flag flag) const = 0;
        virtual const UUID& getInstanceUUID() const = 0;
        virtual const UUID& getPrefabUUID() const = 0;

        virtual size_t getNumChildren() const = 0;
        virtual const IEntityConcreteData& getChild(size_t idx) const = 0;
        virtual size_t getNumComponents() const = 0;
        virtual const std::pair<String, ConfigNode>& getComponent(size_t idx) const = 0;

		bool hasChildWithUUID(const UUID& uuid) const;
	};
	
    class EntityData final : public IEntityConcreteData {
    	friend class EntityDataDelta;
    	
    public:
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

    	const String& getName() const override { return name; }
    	const String& getPrefab() const override { return prefab; }
    	const String& getIcon() const { return icon; }
        uint8_t getFlags() const override { return flags; }
        bool getFlag(Flag flag) const override;
    	const UUID& getInstanceUUID() const override { return instanceUUID; }
    	const UUID& getPrefabUUID() const override { return prefabUUID; }
    	const UUID& getParentUUID() const { return parentUUID; }

    	const Vector<EntityData>& getChildren() const { return children; }
    	Vector<EntityData>& getChildren() { return children; }
        size_t getNumChildren() const override;
        const IEntityConcreteData& getChild(size_t idx) const override;

        const Vector<std::pair<String, ConfigNode>>& getComponents() const { return components; }
    	Vector<std::pair<String, ConfigNode>>& getComponents() { return components; }
        bool hasComponent(const String& componentName) const;
        size_t getNumComponents() const override;
        const std::pair<String, ConfigNode>& getComponent(size_t idx) const override;

        bool fillEntityDataStack(Vector<const EntityData*>& stack, const UUID& entityId) const;

  	    const EntityData* tryGetPrefabUUID(const UUID& uuid) const;
        const EntityData* tryGetInstanceUUID(const UUID& uuid) const;
    	EntityData* tryGetInstanceUUID(const UUID& uuid);
        const ConfigNode& getFieldData(const String& componentName, const String& fieldName) const;
        
    	void setName(String name);
    	void setPrefab(String prefab);
    	void setIcon(String icon);
        bool setFlag(Flag flag, bool value);
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

    	void instantiate(const UUID& uuid);
		void instantiateWith(const EntityData& instance);
	    EntityData instantiateWithAsCopy(const EntityData& instance) const;
    	
        Type getType() const override;

    	void setSceneRoot(bool root);
    	bool isSceneRoot() const;

        std::optional<size_t> getChildIndex(const UUID& uuid) const;

        size_t getSizeBytes() const;

        void generateUUIDs(HashMap<UUID, UUID>& changes);
        void updateComponentUUIDs(const HashMap<UUID, UUID>& changes);

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
