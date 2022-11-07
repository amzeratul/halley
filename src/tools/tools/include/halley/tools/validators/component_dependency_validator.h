#pragma once

#include "halley/core/game/scene_editor_interface.h"
#include "halley/tools/ecs/ecs_data.h"

namespace Halley {
    class ComponentDependencyValidator : public IEntityValidator
    {
    public:
        ComponentDependencyValidator(const ECSData* ecsData);
        Vector<Result> validateEntity(EntityValidator& validator, const EntityData& entityData) override;
    private:
        const ECSData* ecsData = nullptr;
    };
}
