#pragma once
#include "entity_validator.h"

namespace Halley {
    class TransformEntityValidator : public IEntityValidator {
    public:
	    std::vector<Result> validateEntity(EntityValidator& validator, EntityData& entityData) override;
    };

    class AddComponentValidatorActionHandler : public IEntityValidatorActionHandler {
    public:
	    bool canHandle(const ConfigNode& actionData) override;
	    void applyAction(EntityValidator& validator, IEntityEditor& entityEditor, EntityData& entityData, const ConfigNode& actionData) override;
    };
}
