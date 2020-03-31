#include "entity_editor.h"

#include "halley/tools/ecs/ecs_data.h"
using namespace Halley;

EntityEditor::EntityEditor(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(200, 30), UISizer(UISizerType::Vertical))
	, factory(factory)
{
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
			for (auto& c : componentNode.asMap()) {
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

void EntityEditor::loadComponentData(const String& componentType, const ConfigNode& data)
{
	auto componentUI = factory.makeUI("ui/halley/entity_editor_component");
	componentUI->getWidgetAs<UILabel>("componentType")->setText(LocalisedString::fromUserString(componentType));

	auto componentFields = componentUI->getWidget("componentFields");
	componentFields->getSizer().setColumnProportions({{0, 1}});
	
	const auto iter = ecsData->getComponents().find(componentType);
	if (iter != ecsData->getComponents().end()) {
		const auto& componentData = iter->second;
		for (auto& member: componentData.members) {
			String value;
			if (data[member.name].getType() != ConfigNodeType::Undefined) {
				value = data[member.name].asString();
			} else {
				const auto iter = std::find_if(componentData.members.begin(), componentData.members.end(), [&] (const MemberSchema& m) { return m.name == member.name; });
				if (iter != componentData.members.end()) {
					value = iter->defaultValue;
				} else {
					value = "?";
				}
			}
			
			auto label = std::make_shared<UILabel>("", factory.getStyle("labelLight").getTextRenderer("label"), LocalisedString::fromUserString(member.name));
			label->setMaxWidth(100);
			label->setMarquee(true);

			auto labelBox = std::make_shared<UIWidget>("", Vector2f(100, 20), UISizer());
			labelBox->add(label);
			
			auto input = std::make_shared<UITextInput>(factory.getKeyboard(), "", factory.getStyle("input"), value);
			input->setMinSize(Vector2f(60, 25));

			componentFields->add(labelBox, 0, {}, UISizerAlignFlags::CentreVertical);
			componentFields->add(input, 1);
		}
	}
	
	fields->add(componentUI);
}

