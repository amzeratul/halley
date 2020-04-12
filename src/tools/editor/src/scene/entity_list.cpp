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
	list->setDragEnabled(true);
	//list->setDragOutsideEnabled(true);
	add(list, 1);

	setHandle(UIEventType::TreeItemReparented, [=] (const UIEvent& event)
	{
		const auto entityId = event.getStringData();
		const auto newParentId = event.getStringData2();
		const auto childIndex = event.getIntData();

		sceneData->reparentEntity(entityId, newParentId, childIndex);
		sceneEditor->markModified();
	});
}

void EntityList::addEntities(const EntityTree& entity, const String& parentId)
{
	// Root is empty, don't add it
	if (!entity.entityId.isEmpty()) {
		const String name = entity.name + (entity.prefab.isEmpty() ? "" : (" [" + entity.prefab + "]"));
		list->addTreeItem(entity.entityId, parentId, LocalisedString::fromUserString(name));
	}
	
	for (auto& e: entity.children) {
		addEntities(e, entity.entityId);
	}
}

void EntityList::refreshList()
{
	list->clear();

	addEntities(sceneData->getEntityTree(), "");
}

void EntityList::onEntityModified(const String& id, const ConfigNode& node)
{
	list->setLabel(id, LocalisedString::fromUserString(node["name"].asString("")));
}

void EntityList::onEntityAdded(const String& id, const String& parentId, const ConfigNode& data)
{
	list->addTreeItem(id, parentId, LocalisedString::fromUserString(data["name"].asString()));
	list->sortItems();
	list->setSelectedOptionId(id);
}

void EntityList::onEntityRemoved(const String& id, const String& parentId)
{
	list->removeItem(id);
	list->sortItems();
	list->setSelectedOptionId(parentId);
}
