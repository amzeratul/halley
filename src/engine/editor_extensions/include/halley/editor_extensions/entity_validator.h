#pragma once
#include "halley/core/game/scene_editor_interface.h"
#include "halley/data_structures/config_node.h"
#include "halley/text/i18n.h"

namespace Halley {
	class IEntityEditor;
	class EntityData;
	class EntityRef;

    class EntityValidator {
    public:
        explicit EntityValidator(World& world);

        Vector<IEntityValidator::Result> validateEntity(const EntityData& entity, bool recursive);
        void applyAction(IEntityEditor& entityEditor, EntityData& data, const ConfigNode& actionData);
		bool canApplyAction(const IEntityEditor& entityEditor, const EntityData& data, const ConfigNode& actionData);

        void addValidator(std::unique_ptr<IEntityValidator> validator);
        void addActionHandler(std::unique_ptr<IEntityValidatorActionHandler> handler);
        void addDefaults();

        World& getWorld();

    private:
        World& world;

        Vector<std::unique_ptr<IEntityValidator>> validators;
        Vector<std::unique_ptr<IEntityValidatorActionHandler>> handlers;

    	void validateEntity(const EntityData& entity, bool recursive, Vector<IEntityValidator::Result>& result);
    };
}
