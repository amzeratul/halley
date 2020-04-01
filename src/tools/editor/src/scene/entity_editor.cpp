#include "entity_editor.h"
#include "entity_editor_factories.h"
#include "halley/tools/ecs/ecs_data.h"
using namespace Halley;

EntityEditor::EntityEditor(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(200, 30), UISizer(UISizerType::Vertical))
	, factory(factory)
	, context(factory)
{
	addFieldFactories(EntityEditorFactories::getDefaultFactories());
	makeUI();
}

void EntityEditor::setSceneData(ISceneData& scene, ECSData& ecs)
{
	sceneData = &scene;
	ecsData = &ecs;
}

void EntityEditor::showEntity(const String& id)
{
	Expects(sceneData);
	Expects(ecsData);

	fields->clear();
	
	auto data = sceneData->getEntityData(id);
	if (data["components"].getType() == ConfigNodeType::Sequence) {
		for (auto& componentNode: data["components"].asSequence()) {
			for (auto& c: componentNode.asMap()) {
				loadComponentData(c.first, c.second);
			}
		}
	}
}

void EntityEditor::makeUI()
{
	add(factory.makeUI("ui/halley/entity_editor"), 1);
	fields = getWidget("fields");
	fields->setMinSize(Vector2f(300, 20));
}

void EntityEditor::loadComponentData(const String& componentType, ConfigNode& data)
{
	auto componentUI = factory.makeUI("ui/halley/entity_editor_component");
	componentUI->getWidgetAs<UILabel>("componentType")->setText(LocalisedString::fromUserString(componentType));

	auto componentFields = componentUI->getWidget("componentFields");
	componentFields->getSizer().setColumnProportions({{0, 1}});
	
	const auto iter = ecsData->getComponents().find(componentType);
	if (iter != ecsData->getComponents().end()) {
		const auto& componentData = iter->second;
		for (auto& member: componentData.members) {
			const String fieldName = member.name;
			
			auto label = std::make_shared<UILabel>("", factory.getStyle("labelLight").getTextRenderer("label"), LocalisedString::fromUserString(fieldName));
			label->setMaxWidth(100);
			label->setMarquee(true);

			auto labelBox = std::make_shared<UIWidget>("", Vector2f(100, 20), UISizer());
			labelBox->add(label);
			
			componentFields->add(labelBox, 0, {}, UISizerAlignFlags::CentreVertical);
			componentFields->add(createEditField(member.type.name, fieldName, data, member.defaultValue), 1);
		}
	}
	
	fields->add(componentUI);
}

std::shared_ptr<IUIElement> EntityEditor::createEditField(const String& fieldType, const String& fieldName, ConfigNode& componentData, const String& defaultValue)
{
	auto iter = fieldFactories.find(fieldType);
	if (iter != fieldFactories.end()) {
		return iter->second->createField(context, fieldName, componentData, defaultValue);
	} else {
		return std::make_shared<UILabel>("", factory.getStyle("labelLight").getTextRenderer("label"), LocalisedString::fromHardcodedString("N/A"));
	}
}

void EntityEditor::addFieldFactories(std::vector<std::unique_ptr<IComponentEditorFieldFactory>> factories)
{
	for (auto& factory: factories) {
		fieldFactories[factory->getFieldType()] = std::move(factory);
	}
}



