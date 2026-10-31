#pragma once

#include "entity_data_delta.h"

namespace Halley {
    class EntityDataAppliedDelta final : public IEntityConcreteData {
    public:
		EntityDataAppliedDelta() = default;
		EntityDataAppliedDelta(const EntityData& base, const EntityDataDelta& delta, const UUID& rootUUID);

	    Type getType() const override;
	    const String& getName() const override;
	    const String& getPrefab() const override;
	    const String& getVariant() const override;
	    const String& getEnableRules() const override;
	    uint8_t getFlags() const override;
	    bool getFlag(Flag flag) const override;
	    const UUID& getInstanceUUID() const override;
	    const UUID& getPrefabUUID() const override;
	    size_t getNumChildren() const override;
		const IEntityConcreteData& getChild(size_t idx, EntityDataAppliedDelta& buffer) const override;
	    size_t getNumComponents() const override;
		std::tuple<std::string_view, const ConfigNode*, const ConfigNode*> getComponentAndDelta(size_t idx) const override;

    private:
		const EntityData* base = nullptr;
		const EntityDataDelta* delta = nullptr;
		const UUID* rootUUID = nullptr;
    };
}
