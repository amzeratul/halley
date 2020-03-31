#include "entity_list.h"
using namespace Halley;

EntityList::EntityList(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(160, 30), UISizer())
	, factory(factory)
{
	makeUI();
}

void EntityList::clearExceptions()
{
	exceptions.clear();
}

void EntityList::addException(EntityId entityId)
{
	exceptions.insert(entityId);
}

void EntityList::makeUI()
{
	list = std::make_shared<UIList>(getId() + "_list", factory.getStyle("list"));
	add(list, 1);
}

void EntityList::refreshList(const World& world)
{
	list->clear();
	for (auto& e: world.getEntities()) {
		if (exceptions.find(e.getEntityId()) == exceptions.end()) {
			list->addTextItem(e.getUUID().toString(), LocalisedString::fromUserString(e.getName()));
		}
	}
}
