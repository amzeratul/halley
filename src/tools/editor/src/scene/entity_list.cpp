#include "entity_list.h"
using namespace Halley;

EntityList::EntityList(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(200, 30), UISizer())
	, factory(factory)
{
	makeUI();
}

void EntityList::makeUI()
{
	list = std::make_shared<UIList>(getId() + "_list", factory.getStyle("list"));
	list->setSingleClickAccept(false);
	add(list, 1);
}

void EntityList::addEntities(const EntityTree& entity, int depth, const String& parentId)
{
	// Root is empty, don't add it
	if (!entity.entityId.isEmpty()) {
		// HACK
		auto prefix = std::string(depth * 4, ' ');
		
		list->addTextItem(entity.entityId, LocalisedString::fromUserString(prefix + entity.name));
	}
	
	for (auto& e: entity.children) {
		addEntities(e, depth + 1, entity.entityId);
	}
}

void EntityList::refreshList(const ISceneData& sceneData)
{
	list->clear();

	addEntities(sceneData.getEntityTree(), 0, "");
}
