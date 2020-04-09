#include "entity_list.h"

#include "scene_editor_window.h"
using namespace Halley;

EntityList::EntityList(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(200, 30), UISizer())
	, factory(factory)
{
	makeUI();
}

void EntityList::setSceneEditorWindow(SceneEditorWindow& editor)
{
	sceneEditor = &editor;
}

void EntityList::setSceneData(std::shared_ptr<ISceneData> data)
{
	sceneData = std::move(data);
	refreshList();
}

void EntityList::makeUI()
{
	list = std::make_shared<UITreeList>(getId() + "_list", factory.getStyle("treeList"));
	list->setSingleClickAccept(false);
	list->setDrag(true);
	add(list, 1);

	setHandle(UIEventType::TreeItemReparented, [=] (const UIEvent& event)
	{
		const auto entityId = event.getStringData();
		const auto newParentId = event.getStringData2();
		const auto childIndex = event.getIntData();

		sceneData->reparentEntity(entityId, newParentId, childIndex);

		//sceneEditor->onEntityModified();
	});
}

void EntityList::addEntities(const EntityTree& entity, int depth, const String& parentId)
{
	// Root is empty, don't add it
	if (!entity.entityId.isEmpty()) {
		list->addTreeItem(entity.entityId, parentId, LocalisedString::fromUserString(entity.name));
	}
	
	for (auto& e: entity.children) {
		addEntities(e, depth + 1, entity.entityId);
	}
}

void EntityList::refreshList()
{
	list->clear();

	addEntities(sceneData->getEntityTree(), 0, "");
}
