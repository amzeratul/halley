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
					result.emplace_back("Entity has no Transform2D component, but some of its children do.", Action("Add Component", AddComponentValidatorActionHandler::makeAction("Transform2D")));
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

ConfigNode AddComponentValidatorActionHandler::makeAction(String componentName)
{
	ConfigNode::MapType action;
	action["action"] = "addComponent";
	action["component"] = std::move(componentName);
	return action;
}

bool ModifyFieldsValidatorActionHandler::canHandle(const ConfigNode& actionData)
{
	return actionData["action"].asString() == "modifyField" || actionData["action"].asString() == "modifyFields";
}

void ModifyFieldsValidatorActionHandler::applyAction(EntityValidator& validator, IEntityEditor& entityEditor, EntityData& entityData, const ConfigNode& actionData)
{
	if (actionData["action"].asString() == "modifyField") {
		applyEntry(entityEditor, entityData, actionData);
	} else if (actionData["action"].asString() == "modifyFields") {
		for (const auto& entry: actionData["entries"].asSequence()) {
			applyEntry(entityEditor, entityData, entry);
		}
	}
}

ConfigNode ModifyFieldsValidatorActionHandler::makeAction(String componentName, String fieldName, ConfigNode fieldData)
{
	ConfigNode::MapType result;

	result["action"] = "modifyField";
	result["component"] = std::move(componentName);
	result["field"] = std::move(fieldName);
	result["data"] = std::move(fieldData);

	return result;
}

void ModifyFieldsValidatorActionHandler::applyEntry(IEntityEditor& entityEditor, EntityData& entityData, const ConfigNode& entry)
{
	const auto component = entry["component"].asString();
	const auto field = entry["field"].asString();

	bool found = false;
	for (auto& comp: entityData.getComponents()) {
		if (comp.first == component) {
			found = applyField(comp.second, field, entry["data"]);
			break;
		}
	}

	if (found) {
		entityEditor.onFieldChangedByGizmo(component, field);
	}
}

bool ModifyFieldsValidatorActionHandler::applyField(ConfigNode& dst, const String& fieldName, const ConfigNode& entryData)
{
	dst.ensureType(ConfigNodeType::Map);

	const auto slashPos = fieldName.find('/');
	if (slashPos != String::npos) {
		auto p0 = fieldName.substr(0, slashPos);
		auto p1 = fieldName.mid(slashPos + 1);
		return applyField(dst[p0], p1, entryData);
	} else {
		dst[fieldName] = ConfigNode(entryData);
		return true;
	}
}
