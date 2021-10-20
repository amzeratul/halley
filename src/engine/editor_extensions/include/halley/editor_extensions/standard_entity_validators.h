#pragma once
#include "entity_validator.h"

namespace Halley {
    class TransformEntityValidator : public IEntityValidator {
    public:
	    std::vector<Result> validateEntity(EntityValidator& validator, EntityData& entityData) override;
    };
}
