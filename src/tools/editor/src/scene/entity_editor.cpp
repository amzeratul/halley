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

	background->clear();
	
	auto data = sceneData->getEntityData(id);
	if (data["components"].getType() == ConfigNodeType::Sequence) {
		for (auto& componentNode: data["components"].asSequence()) {
			for (auto& c : componentNode.asMap()) {
				const auto& componentType = c.first;
				const auto& componentData = c.second;

				background->add(std::make_shared<UIImage>(Sprite().setImage(factory.getResources(), "ui/title_capsule.png").setColour(Colour4f::fromString("#9C5BB5"))), 0);
			}
		}
	}
}

void EntityEditor::makeUI()
{
	auto sprite = Sprite().setImage(factory.getResources(), "halley_ui/ui_window.png").setColour(Colour4f::fromString("#201427"));
	background = std::make_shared<UIImage>(sprite, UISizer(UISizerType::Vertical), Vector4f(4, 4, 4, 4));
	add(background, 1);
}

