#include "entity_list.h"


#include "entity_icons.h"
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
	icons = &sceneEditor->getEntityIcons();
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

		auto [prevParent, prevIndex] = sceneData->reparentEntity(entityId, newParentId, childIndex);
		sceneEditor->onEntityMoved(entityId, prevParent, prevIndex, newParentId, childIndex);
	});
}

void EntityList::addEntities(const EntityTree& entity, const String& parentId)
{
	// Root is empty, don't add it
	if (!entity.entityId.isEmpty()) {
		addEntity(entity.name, entity.entityId, parentId, -1, entity.prefab, entity.icon);
	}

	for (auto& e: entity.children) {
		addEntities(e, entity.entityId);
	}
}

void EntityList::addEntity(const String& name, const String& id, const String& parentId, int childIndex, const String& prefab, const String& icon)
{
	const bool isPrefab = !prefab.isEmpty();
	const auto [displayName, displayIcon] = getEntityNameAndIcon(name, icon, prefab);
	const size_t idx = childIndex >= 0 ? static_cast<size_t>(childIndex) : std::numeric_limits<size_t>::max();
	list->addTreeItem(id, parentId, idx, LocalisedString::fromUserString(displayName), isPrefab ? "labelSpecial" : "label", displayIcon, isPrefab);
}

std::pair<String, Sprite> EntityList::getEntityNameAndIcon(const EntityData& data) const
{
	return getEntityNameAndIcon(data.getName(), data.getIcon(), data.getPrefab());
}

std::pair<String, Sprite> EntityList::getEntityNameAndIcon(const String& name, const String& icon, const String& prefabName) const
{
	if (!prefabName.isEmpty()) {
		const auto prefab = sceneEditor->getGamePrefab(prefabName);
		if (prefab) {
			return { prefab->getPrefabName()/* + " [" + prefabName + "]"*/, icons->getIcon(prefab->getPrefabIcon()) };
		} else {
			return { "Missing prefab! [" + prefabName + "]", icons->getIcon("") };
		}
	} else {
		return { name.isEmpty() ? String("Unnamed Entity") : name, icons->getIcon(icon) };
	}
}

void EntityList::refreshList()
{
	const auto prevId = list->getSelectedOptionId();

	list->setScrollToSelection(false);
	list->clear();
	addEntities(sceneData->getEntityTree(), "");
	layout();
	list->setScrollToSelection(true);

	list->setSelectedOptionId(prevId);
}

void EntityList::refreshNames()
{
	refreshList();
}

void EntityList::onEntityModified(const String& id, const EntityData& node)
{
	const auto [name, icon] = getEntityNameAndIcon(node);
	list->setLabel(id, LocalisedString::fromUserString(name), icon);
}

void EntityList::onEntityAdded(const String& id, const String& parentId, int childIndex, const EntityData& data)
{
	addEntityTree(parentId, childIndex, data);
	list->sortItems();
	layout();
	list->setSelectedOptionId(id);
}

void EntityList::addEntityTree(const String& parentId, int childIndex, const EntityData& data)
{
	const auto& curId = data.getInstanceUUID().toString();
	addEntity(data.getName(), curId, parentId, childIndex, data.getPrefab(), data.getIcon());
	for (const auto& child: data.getChildren()) {
		addEntityTree(curId, -1, child);
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
