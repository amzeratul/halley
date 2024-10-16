#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/maths/uuid.h"
#include <set>
#include "entity_data.h"
#include "halley/bytes/config_node_serializer_base.h"

namespace Halley {
	class IDataInterpolatorSetRetriever;

	class EntityDataDelta final : public IEntityData {
		friend class EntityData;
		
	public:
        class Options {
        public:
            Options();
        	
	        bool preserveChildOrder = false;
	        bool preserveComponentOrder = false;
			bool shallow = false;
            bool deltaComponents = true;
			bool allowNonSerializable = true;
			bool omitEmptyComponents = false;
			bool ignoreNameChangesInInstances = false;
        	HashSet<String> ignoreComponents;
        	HashSet<String> ignoreInsertComponents;
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
		const std::optional<String>& getVariant() const { return variant; }
		const std::optional<String>& getEnableRules() const { return enableRules; }
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
    	
        Type getType() const override;

		void instantiate(const UUID& uuid);
		EntityDataDelta instantiateAsCopy(const UUID& uuid) const;

		bool modifiesTheSameAs(const EntityDataDelta& other) const;

		ConfigNode toConfigNode() const;
		String toYAML() const;

		bool operator==(const EntityDataDelta& other) const;
		bool operator!=(const EntityDataDelta& other) const;

		void sanitize(const WorldReflection& worldReflection, int mask);

	private:
    	std::optional<String> name;
    	std::optional<String> prefab;
		std::optional<String> icon;
		std::optional<String> variant;
		std::optional<String> enableRules;
		std::optional<uint8_t> flags;
    	std::optional<UUID> instanceUUID;
    	std::optional<UUID> prefabUUID;
		std::optional<UUID> parentUUID;

		bool deserializeChildrenComponentsAsDeltas = true;

		Vector<std::pair<String, ConfigNode>> componentsChanged;// Also includes components added
		Vector<String> componentsRemoved;
		Vector<String> componentOrder;

		Vector<EntityData> childrenAdded;
		Vector<std::pair<UUID, EntityDataDelta>> childrenChanged;
		Vector<UUID> childrenRemoved;
		Vector<UUID> childrenOrder;

        enum class FieldId {
        	RESERVED, // DO NOT USE THIS, it's reserved in case the format needs to change
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
			Flags,
			Variant,
			EnableRules
        };

    	static uint32_t getFieldBit(FieldId id);
    	static void setFieldPresent(uint32_t& value, FieldId id, bool present);
    	static bool isFieldPresent(uint32_t value, FieldId id);

		uint32_t getFieldsPresent() const;

		Vector<std::pair<String, ConfigNode>> getComponentEmptyStructure() const;
	};

	class SceneDataDelta {
	public:
		void addEntity(UUID entityId, EntityDataDelta delta);
		const Vector<std::pair<UUID, EntityDataDelta>>& getEntities() const;

		void serialize(Serializer& s) const;
    	void deserialize(Deserializer& s);

		ConfigNode toConfigNode() const;

	private:
		Vector<std::pair<UUID, EntityDataDelta>> entities;
	};

	class Resources;
	template<>
	class ConfigNodeSerializer<EntityDataDelta> {
	public:
		ConfigNode serialize(const EntityDataDelta& entityData, const EntitySerializationContext& context);
		EntityDataDelta deserialize(const EntitySerializationContext& context, const ConfigNode& node);
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, EntityDataDelta& target);
	};
}
