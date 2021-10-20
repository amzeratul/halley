#pragma once
#include "halley/data_structures/config_node.h"
#include "halley/text/i18n.h"

namespace Halley {
	class EntityData;
	class EntityRef;

    class EntityValidator;

    class IEntityValidator {
    public:
        struct Action {
	        LocalisedString label;
            ConfigNode actionData;
        };

        struct Result {
            LocalisedString errorMessage;
            std::vector<String> suggestedActions;

            bool operator==(const Result& other) const;
            bool operator!=(const Result& other) const;
        };

    	virtual ~IEntityValidator() = default;

        virtual std::vector<Result> validateEntity(EntityValidator& validator, EntityRef entity) = 0;
    };

    class IEntityValidatorActionHandler {
    public:
        virtual ~IEntityValidatorActionHandler() = default;

        virtual bool canHandle(const ConfigNode& actionData) = 0;
        virtual void applyAction(EntityValidator& validator, EntityData& data, const ConfigNode& actionData) = 0;
    };

    class EntityValidator {
    public:
        std::vector<IEntityValidator::Result> validateEntity(EntityRef& entity);
        void applyAction(EntityData& data, const ConfigNode& actionData);

        void addValidator(std::unique_ptr<IEntityValidator> validator);
        void addActionHandler(std::unique_ptr<IEntityValidatorActionHandler> handler);

    private:
        std::vector<std::unique_ptr<IEntityValidator>> validators;
        std::vector<std::unique_ptr<IEntityValidatorActionHandler>> handlers;
    };
}
