#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/maths/uuid.h"
#include <set>
#include "entity_data.h"

namespace Halley {
	class IDataInterpolatorSetRetriever;

	class EntityDataDelta : public IEntityData {
		friend class EntityData;
		
	public:
        class Options {
        public:
            Options();
        	
	        bool preserveOrder = false;
			bool shallow = false;
            bool deltaComponents = true;
        	std::set<String> ignoreComponents;
			IDataInterpolatorSetRetriever* interpolatorSet = nullptr;
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
		const std::optional<uint8_t>& getFlags() const { return flags; }
		const std::optional<UUID>& getPrefabUUID() const { return prefabUUID; }
		const std::optional<UUID>& getInstanceUUID() const { return instanceUUID; }
		void setInstanceUUID(const UUID& uuid);
		void setPrefabUUID(const UUID& uuid);
		void randomiseInstanceUUIDs();

		Vector<std::pair<String, ConfigNode>>& getComponentsChanged() { return componentsChanged; }
		const Vector<std::pair<String, ConfigNode>>& getComponentsChanged() const { return componentsChanged; }
		const Vector<String>& getComponentsRemoved() const { return componentsRemoved; }
		
		const Vector<EntityData>& getChildrenAdded() const { return childrenAdded; }
		const Vector<std::pair<UUID, EntityDataDelta>>& getChildrenChanged() const { return childrenChanged; }
		const Vector<UUID>& getChildrenRemoved() const { return childrenRemoved; }

		bool isSimpleDelta() const;
    	
        bool isDelta() const override;
        const EntityData& asEntityData() const override;
        const EntityDataDelta& asEntityDataDelta() const override;

		bool modifiesTheSameAs(const EntityDataDelta& other) const;

		ConfigNode toConfigNode() const;
		String toYAML() const;

	private:
    	std::optional<String> name;
    	std::optional<String> prefab;
		std::optional<String> icon;
		std::optional<uint8_t> flags;
    	std::optional<UUID> instanceUUID;
    	std::optional<UUID> prefabUUID;
		std::optional<UUID> parentUUID;

		Vector<std::pair<String, ConfigNode>> componentsChanged;// Also includes components added
		Vector<String> componentsRemoved;
		Vector<String> componentOrder;

		Vector<EntityData> childrenAdded;
		Vector<std::pair<UUID, EntityDataDelta>> childrenChanged;
		Vector<UUID> childrenRemoved;
		Vector<UUID> childrenOrder;

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
        	Icon,
			Flags
        };

    	static uint16_t getFieldBit(FieldId id);
    	static void setFieldPresent(uint16_t& value, FieldId id, bool present);
    	static bool isFieldPresent(uint16_t value, FieldId id);

		uint16_t getFieldsPresent() const;

		Vector<std::pair<String, ConfigNode>> getComponentEmptyStructure() const;
	};

	class SceneDataDelta {
	public:
		void addEntity(UUID entityId, EntityDataDelta delta);
		const Vector<std::pair<UUID, EntityDataDelta>>& getEntities() const;

		void serialize(Serializer& s) const;
    	void deserialize(Deserializer& s);

	private:
		Vector<std::pair<UUID, EntityDataDelta>> entities;
	};
}
