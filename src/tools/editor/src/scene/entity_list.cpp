#include "entity_list.h"

#include "halley/ui/ui_factory.h"
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
		addEntity(entity.name, entity.entityId, parentId, "", entity.prefab);
	}

	for (auto& e: entity.children) {
		addEntities(e, entity.entityId);
	}
}

void EntityList::addEntity(const String& name, const String& id, const String& parentId, const String& afterSiblingId, const String& prefab)
{
	const bool isPrefab = !prefab.isEmpty();
	list->addTreeItem(id, parentId, afterSiblingId, LocalisedString::fromUserString(getEntityName(name, prefab)), isPrefab ? "labelSpecial" : "label", isPrefab);
}

String EntityList::getEntityName(const ConfigNode& data) const
{
	return getEntityName(data["name"].asString(""), data["prefab"].asString(""));
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
		if (name.isEmpty()) {
			return "Unnamed Entity";
		} else {
			return name;
		}
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

void EntityList::onEntityAdded(const String& id, const String& parentId, const String& afterSiblingId, const ConfigNode& data)
{
	addEntityTree(parentId, afterSiblingId, data);
	list->sortItems();
	list->setSelectedOptionId(id);
}

void EntityList::addEntityTree(const String& parentId, const String& afterSiblingId, const ConfigNode& data)
{
	const auto& curId = data["uuid"].asString();
	addEntity(data["name"].asString(""), curId, parentId, afterSiblingId, data["prefab"].asString(""));
	if (data["children"].getType() == ConfigNodeType::Sequence) {
		for (const auto& child: data["children"]) {
			addEntityTree(curId, "", child);
		}
	}
}

void EntityList::onEntityRemoved(const String& id, const String& parentId)
{
	list->removeItem(id);
	list->sortItems();
	list->setSelectedOptionId(parentId);
}

void EntityList::select(const String& id)
{
	list->setSelectedOptionId(id);
}

bool EntityList::onKeyPress(KeyboardKeyPress key)
{
	const auto& curEntity = list->getSelectedOptionId();
	
	if (key.is(KeyCode::Delete)) {
		sceneEditor->removeEntity(curEntity);
		return true;
	}

	if (key.is(KeyCode::C, KeyMods::Ctrl)) {
		sceneEditor->copyEntityToClipboard(curEntity);
		return true;
	}

	if (key.is(KeyCode::X, KeyMods::Ctrl)) {
		sceneEditor->copyEntityToClipboard(curEntity);
		sceneEditor->removeEntity(curEntity);
		return true;
	}

	if (key.is(KeyCode::V, KeyMods::Ctrl)) {
		sceneEditor->pasteEntityFromClipboard(curEntity);
		return true;
	}

	if (key.is(KeyCode::D, KeyMods::Ctrl)) {
		sceneEditor->duplicateEntity(curEntity);
		return true;
	}

	return false;
}
