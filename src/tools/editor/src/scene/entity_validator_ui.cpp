#include "entity_validator_ui.h"

#include "halley/editor_extensions/entity_validator.h"
using namespace Halley;

EntityValidatorUI::EntityValidatorUI(String id, UIFactory& factory)
	: UIWidget(std::move(id), {}, UISizer())
	, factory(factory)
{
	factory.loadUI(*this, "ui/halley/entity_validator");
	setActive(false);
}

void EntityValidatorUI::setValidator(EntityValidator& v)
{
	validator = &v;
	refresh();
}

void EntityValidatorUI::setEntity(EntityRef& e)
{
	curEntity = e;
	refresh();
}

void EntityValidatorUI::refresh()
{
	if (!curEntity.isValid() || !validator) {
		return;
	}

	const auto result = validator->validateEntity(curEntity);
	if (result != curResultSet) {
		curResultSet = std::move(result);
		setActive(!curResultSet.empty());

		auto parent = getWidget("validationFields");
		parent->clear();

		for (const auto& result: curResultSet) {
			// TODO: show error message
			// result.errorMessage;
			for (const auto& action: result.suggestedActions) {
				// TODO: add action button
			}
		}
	}
}
