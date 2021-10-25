#pragma once
#include "entity_validator.h"

namespace Halley {
    class TransformEntityValidator : public IEntityValidator {
    public:
	    std::vector<Result> validateEntity(EntityValidator& validator, const EntityData& entityData) override;
    };

    class AddComponentValidatorActionHandler : public IEntityValidatorActionHandler {
    public:
	    bool canHandle(const ConfigNode& actionData) override;
        bool canApplyAction(EntityValidator& validator, const IEntityEditor& entityEditor, const EntityData& entityData, const ConfigNode& actionData) override;
	    void applyAction(EntityValidator& validator, IEntityEditor& entityEditor, EntityData& entityData, const ConfigNode& actionData) override;

        static ConfigNode makeAction(String componentName);
    };

    class RemoveComponentValidatorActionHandler : public IEntityValidatorActionHandler {
    public:
	    bool canHandle(const ConfigNode& actionData) override;
        bool canApplyAction(EntityValidator& validator, const IEntityEditor& entityEditor, const EntityData& entityData, const ConfigNode& actionData) override;
	    void applyAction(EntityValidator& validator, IEntityEditor& entityEditor, EntityData& entityData, const ConfigNode& actionData) override;

        static ConfigNode makeAction(String componentName);
    };

    class ModifyFieldsValidatorActionHandler : public IEntityValidatorActionHandler {
    public:
	    bool canHandle(const ConfigNode& actionData) override;
        bool canApplyAction(EntityValidator& validator, const IEntityEditor& entityEditor, const EntityData& entityData, const ConfigNode& actionData) override;
	    void applyAction(EntityValidator& validator, IEntityEditor& entityEditor, EntityData& entityData, const ConfigNode& actionData) override;

        static ConfigNode makeAction(String componentName, String fieldName, ConfigNode fieldData);

    private:
        void applyEntry(IEntityEditor& entityEditor, EntityData& entityData, const ConfigNode& entryData);
        bool applyField(ConfigNode& dst, const String& fieldName, const ConfigNode& entryData);
    };
}
