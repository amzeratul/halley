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

void EntityValidatorUI::setEntity(EntityData& e, IEntityEditor& editor)
{
	curEntity = &e;
	entityEditor = &editor;
	refresh();
}

void EntityValidatorUI::refresh()
{
	if (!curEntity || !validator) {
		return;
	}

	auto result = validator->validateEntity(*curEntity);
	if (result != curResultSet) {
		curResultSet = std::move(result);
		setActive(!curResultSet.empty());

		auto parent = getWidget("validationFields");
		parent->clear();

		for (const auto& curResult: curResultSet) {
			auto label = std::make_shared<UILabel>("", factory.getStyle("labelLight"), curResult.errorMessage);
			label->setMaxWidth(300.0f);
			parent->add(label);

			for (const auto& action: curResult.suggestedActions) {
				auto button = std::make_shared<UIButton>("action", factory.getStyle("buttonThin"), action.label);
				button->setHandle(UIEventType::ButtonClicked, [this, actionData = ConfigNode(action.actionData)] (const UIEvent& event)
				{
					Concurrent::execute(Executors::getMainThread(), [=] () {
						validator->applyAction(*entityEditor, *curEntity, actionData);
						entityEditor->reloadEntity();
					});
				});
				parent->add(button);
			}
		}
	}
}
