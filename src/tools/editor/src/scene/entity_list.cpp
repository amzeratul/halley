#include "entity_list.h"
using namespace Halley;

EntityList::EntityList(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(160, 30), UISizer())
	, factory(factory)
{
	makeUI();
}

void EntityList::makeUI()
{
	add(std::make_shared<UIList>(getId() + "_list", factory.getStyle("list")));
}
