#include "standard_entity_validators.h"
#include "halley/entity/entity.h"
#include "halley/entity/world.h"

#define DONT_INCLUDE_HALLEY_HPP
#include "component_editor_context.h"
#include "halley/entity/components/transform_2d_component.h"

using namespace Halley;

std::vector<IEntityValidator::Result> TransformEntityValidator::validateEntity(EntityValidator& validator, EntityData& entityData)
{
	std::vector<Result> result;

	const auto entity = validator.getWorld().findEntity(entityData.getInstanceUUID());
	if (entity->isValid()) {
		const bool hasTransform = entity->hasComponent<Transform2DComponent>();

		if (!hasTransform) {
			for (auto c: entity->getChildren()) {
				if (c.hasComponent<Transform2DComponent>()) {
					ConfigNode::MapType action;
					action["action"] = "addComponent";
					action["component"] = "Transform2D";
					result.emplace_back("Entity has no Transform2D component, but some of its children do.", Action("Add Component", std::move(action)));
				}
			}
		}
	}

	return result;
}

bool AddComponentValidatorActionHandler::canHandle(const ConfigNode& actionData)
{
	return actionData["action"].asString() == "addComponent" && actionData.hasKey("component");
}

void AddComponentValidatorActionHandler::applyAction(EntityValidator& validator, IEntityEditor& entityEditor, EntityData& entityData, const ConfigNode& actionData)
{
	const auto compType = actionData["component"].asString();
	entityEditor.addComponent(compType, ConfigNode::MapType());
}
