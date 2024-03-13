#pragma once
#include "entity_data.h"

namespace Halley {
	class EntityDataInstanced final : public IEntityConcreteData {
	public:
		EntityDataInstanced() = default;
		EntityDataInstanced(const EntityData& prefabData, const IEntityConcreteData& instanceData);
		EntityDataInstanced(const EntityData& prefabData, const UUID& rootInstanceUUID);

		Type getType() const override;

		const String& getName() const override;
		const String& getPrefab() const override;
		const String& getPrefabInstanced() const override;
		const String& getVariant() const override;
		uint8_t getFlags() const override;
		bool getFlag(Flag flag) const override;
		const UUID& getInstanceUUID() const override;
		const UUID& getPrefabUUID() const override;
		size_t getNumChildren() const override;
		const IEntityConcreteData& getChild(size_t idx) const override;
		size_t getNumComponents() const override;
		const std::pair<String, ConfigNode>& getComponent(size_t idx) const override;

	private:
		const EntityData* prefabData = nullptr;
		UUID instanceUUID;
		uint8_t flags = 0;

		Vector<EntityDataInstanced> children;
		HashMap<String, std::pair<String, ConfigNode>> componentOverrides;
	};
}
