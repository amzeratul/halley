#include "entity_editor.h"
using namespace Halley;

EntityEditor::EntityEditor(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(200, 30), UISizer())
	, factory(factory)
{
	makeUI();
}

void EntityEditor::makeUI()
{
	auto sprite = Sprite().setImage(factory.getResources(), "halley_ui/ui_window.png").setColour(Colour4f::fromString("#201427"));
	add(std::make_shared<UIImage>(sprite), 1);
}
