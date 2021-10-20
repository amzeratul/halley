#include "entity_validator.h"
#include "halley/entity/entity.h"

using namespace Halley;

std::vector<IEntityValidator::Result> EntityValidator::validateEntity(EntityRef& entity)
{
	std::vector<IEntityValidator::Result> result;

	// TODO

	return result;
}

void EntityValidator::applyAction(EntityData& data, const ConfigNode& actionData)
{
	// TODO
}

void EntityValidator::addValidator(std::unique_ptr<IEntityValidator> validator)
{
	validators.push_back(std::move(validator));
}

void EntityValidator::addActionHandler(std::unique_ptr<IEntityValidatorActionHandler> handler)
{
	handlers.push_back(std::move(handler));
}
