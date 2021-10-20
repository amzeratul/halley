#include "entity_validator.h"
#include "halley/entity/entity.h"
#include "halley/support/logger.h"

using namespace Halley;

bool IEntityValidator::Result::operator==(const Result& other) const
{
	return errorMessage == other.errorMessage && suggestedActions == other.suggestedActions;
}

bool IEntityValidator::Result::operator!=(const Result& other) const
{
	return errorMessage != other.errorMessage || suggestedActions != other.suggestedActions;
}

std::vector<IEntityValidator::Result> EntityValidator::validateEntity(EntityData& entity)
{
	std::vector<IEntityValidator::Result> result;

	for (const auto& validator: validators) {
		auto r = validator->validateEntity(*this, entity);
		result.insert(result.end(), r.begin(), r.end());
	}

	return result;
}

void EntityValidator::applyAction(EntityData& data, const ConfigNode& actionData)
{
	for (const auto& handler: handlers) {
		if (handler->canHandle(actionData)) {
			handler->applyAction(*this, data, actionData);
			return;
		}
	}
	Logger::logError("No handler found for EntityValidator action.");
}

void EntityValidator::addValidator(std::unique_ptr<IEntityValidator> validator)
{
	validators.push_back(std::move(validator));
}

void EntityValidator::addActionHandler(std::unique_ptr<IEntityValidatorActionHandler> handler)
{
	handlers.push_back(std::move(handler));
}
