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
	list->setSingleRoot(sceneData->isSingleRoot());
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
		sceneEditor->onEntityMoved(entityId);
	});
}

void EntityList::addEntities(const EntityTree& entity, const String& parentId)
{
	// Root is empty, don't add it
	if (!entity.entityId.isEmpty()) {
		addEntity(entity.name, entity.entityId, parentId, entity.prefab);
	}
	
	for (auto& e: entity.children) {
		addEntities(e, entity.entityId);
	}
}

void EntityList::addEntity(const String& name, const String& id, const String& parentId, const String& prefab)
{
	const bool isPrefab = !prefab.isEmpty();
	list->addTreeItem(id, parentId, LocalisedString::fromUserString(getEntityName(name, prefab)), isPrefab ? "labelSpecial" : "label", isPrefab);
}

String EntityList::getEntityName(const ConfigNode& data) const
{
	return getEntityName(data["name"].asString("?"), data["prefab"].asString(""));
}

String EntityList::getEntityName(const String& name, const String& prefabName) const
{
	if (!prefabName.isEmpty()) {
		const auto prefab = sceneEditor->getGamePrefab(prefabName);
		if (prefab) {
			return prefab->getRoot()["name"].asString() + " [" + prefabName + "]";
		} else {
			return "Missing prefab! [" + prefabName + "]";
		}
	} else {
		return name;
	}
}

void EntityList::refreshList()
{
	list->clear();

	addEntities(sceneData->getEntityTree(), "");
}

void EntityList::onEntityModified(const String& id, const ConfigNode& node)
{
	list->setLabel(id, LocalisedString::fromUserString(getEntityName(node)));
}

void EntityList::onEntityAdded(const String& id, const String& parentId, const ConfigNode& data)
{
	addEntity(data["name"].asString(""), id, parentId, data["prefab"].asString(""));
	list->sortItems();
	list->setSelectedOptionId(id);
}

void EntityList::onEntityRemoved(const String& id, const String& parentId)
{
	list->removeItem(id);
	list->sortItems();
	list->setSelectedOptionId(parentId);
}
