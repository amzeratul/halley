#include "halley/tools/validators/component_dependency_validator.h"

#include "halley/editor_extensions/entity_validator.h"
#include "halley/editor_extensions/standard_entity_validators.h"
#include "halley/entity/entity.h"
#include "halley/entity/world.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

ComponentDependencyValidator::ComponentDependencyValidator(const ECSData* ecsData)
	: ecsData(ecsData)
{

}

Vector<IEntityValidator::Result> ComponentDependencyValidator::validateEntity(EntityValidator& validator, const EntityData& entityData)
{
	Vector<Result> result;

	const auto& componentSchemas = ecsData->getComponents();
	for (auto& component : entityData.getComponents()) {
		const auto iter = componentSchemas.find(component.first);
		if (iter == componentSchemas.end()) {
			result.emplace_back(Severity::Error, "Component " + component.first + " does not exist.", IEntityValidator::Action("Remove Component", RemoveComponentValidatorActionHandler::makeAction(component.first)));
			continue;
		}

		const auto& componentSchema = iter->second;
		for (auto& dependsOn : componentSchema.componentDependencies) {
			const bool hasDepedency = std_ex::contains_if(entityData.getComponents(), [&](const std::pair<String, ConfigNode>& comp) { return comp.first == dependsOn; });
			if (!hasDepedency) {
				result.emplace_back(Severity::Error, component.first + " depends on " + dependsOn + " which is missing.");
			}
		}

		//for (auto& dependsOn : componentSchema.componentDependenciesInAncestors) { // TODO disabled for now.. does not work as intended
		//	const bool hasDepedency = std_ex::contains_if(entityData.getComponents(), [&](const std::pair<String, ConfigNode>& comp) { return comp.first == dependsOn; });
		//	if (!hasDepedency) {
		//		bool hasDepedencyInWalker = false;

		//		const auto entityDataStack = validator.getEntityDataStack(entityData.getInstanceUUID());
		//		for (const auto& stackWalker : entityDataStack) {
		//			hasDepedencyInWalker = std_ex::contains_if(stackWalker->getComponents(), [&](const std::pair<String, ConfigNode>& comp) { return comp.first == dependsOn; });
		//			if (hasDepedencyInWalker) {
		//				break;
		//			}
		//		}
		//		if (!hasDepedencyInWalker) {
		//			result.emplace_back(Severity::Error, component.first + " depends on " + dependsOn + " which is missing on itself or in any of the parents.");
		//		}
		//	}
		//}
	}

	return result;
}
