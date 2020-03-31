#include "entity_editor.h"
using namespace Halley;

EntityEditor::EntityEditor(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(200, 30), UISizer(UISizerType::Vertical))
	, factory(factory)
{
	makeUI();
}

void EntityEditor::setSceneData(ISceneData& data)
{
	sceneData = &data;
}

void EntityEditor::showEntity(const String& id)
{
	Expects(sceneData);

	auto fields = getWidget("fields");
	fields->clear();
	
	auto data = sceneData->getEntityData(id);
	if (data["components"].getType() == ConfigNodeType::Sequence) {
		for (auto& componentNode: data["components"].asSequence()) {
			for (auto& c : componentNode.asMap()) {
				const auto& componentType = c.first;
				const auto& componentData = c.second;

				auto componentUI = factory.makeUI("ui/halley/entity_editor_component");
				componentUI->getWidgetAs<UILabel>("componentType")->setText(LocalisedString::fromUserString(componentType));
				fields->add(componentUI);
			}
		}
	}
}

void EntityEditor::makeUI()
{
	add(factory.makeUI("ui/halley/entity_editor"), 1);
}

