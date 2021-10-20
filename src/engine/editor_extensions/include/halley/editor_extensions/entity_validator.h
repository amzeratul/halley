#pragma once
#include "halley/core/game/scene_editor_interface.h"
#include "halley/data_structures/config_node.h"
#include "halley/text/i18n.h"

namespace Halley {
	class EntityData;
	class EntityRef;

    class EntityValidator {
    public:
        std::vector<IEntityValidator::Result> validateEntity(EntityData& entity);
        void applyAction(EntityData& data, const ConfigNode& actionData);

        void addValidator(std::unique_ptr<IEntityValidator> validator);
        void addActionHandler(std::unique_ptr<IEntityValidatorActionHandler> handler);

    private:
        std::vector<std::unique_ptr<IEntityValidator>> validators;
        std::vector<std::unique_ptr<IEntityValidatorActionHandler>> handlers;
    };
}
