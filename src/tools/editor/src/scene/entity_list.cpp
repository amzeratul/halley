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
	sceneEditorWindow = &editor;
	icons = &sceneEditorWindow->getEntityIcons();
}

void EntityList::setSceneData(std::shared_ptr<ISceneData> data)
{
	sceneData = std::move(data);
	if (sceneData) {
		list->setSingleRoot(sceneData->isSingleRoot());
	}
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
		sceneEditorWindow->onEntityMoved(entityId, prevParent, static_cast<int>(prevIndex), newParentId, childIndex);
	});

	setHandle(UIEventType::ListItemRightClicked, [=] (const UIEvent& event)
	{
		openContextMenu(event.getStringData());
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
		const auto prefab = sceneEditorWindow->getGamePrefab(prefabName);
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
	if (sceneData) {
		addEntities(sceneData->getEntityTree(), "");
	}
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

void EntityList::onEntityRemoved(const String& id, const String& newSelectionId)
{
	list->removeItem(id);
	list->sortItems();
	layout();
	list->setSelectedOption(-1);
	list->setSelectedOptionId(newSelectionId);
}

void EntityList::select(const String& id)
{
	list->setSelectedOptionId(id);
}

UUID EntityList::getEntityUnderCursor() const
{
	const auto item = list->getItemUnderCursor();
	if (item) {
		return UUID(item->getId());
	} else {
		return UUID();
	}
}

String EntityList::getCurrentSelection() const
{
	return list->getSelectedOptionId();
}

void EntityList::openContextMenu(const String& entityId)
{
	auto menuOptions = std::vector<UIPopupMenuItem>();
	auto makeEntry = [&] (const String& id, const String& text, const String& toolTip, const String& icon, bool enabled = true)
	{
		auto iconSprite = Sprite().setImage(factory.getResources(), "entity_icons/" + (icon.isEmpty() ? "empty.png" : icon));
		menuOptions.push_back(UIPopupMenuItem(id, LocalisedString::fromHardcodedString(text), std::move(iconSprite), LocalisedString::fromHardcodedString(toolTip)));
		menuOptions.back().enabled = enabled;
	};
	
	makeEntry("add_entity_sibling", "Add Entity", "Adds an empty entity as a sibling of this one.", "");
	makeEntry("add_entity_child", "Add Entity (Child)", "Adds an empty entity as a child of this one.", "");
	makeEntry("add_prefab_sibling", "Add Prefab", "Adds a prefab as a sibling of this entity.", "");
	makeEntry("add_prefab_child", "Add Prefab (Child)", "Adds a prefab as a child of this entity.", "");
	menuOptions.emplace_back();
	makeEntry("copy", "Copy", "Copy entity to clipboard [Ctrl+C]", "copy.png");
	makeEntry("cut", "Cut", "Cut entity to clipboard [Ctrl+X]", "cut.png");
	makeEntry("paste_sibling", "Paste", "Paste entity as a sibling of the current one. [Ctrl+V]", "paste.png");
	makeEntry("paste_child", "Paste (Child)", "Paste entity as a child of the current one.", "");
	menuOptions.emplace_back();
	makeEntry("duplicate", "Duplicate", "Duplicate entity [Ctrl+D]", "");
	makeEntry("delete", "Delete", "Delete entity [Del]", "delete.png");
	
	auto menu = std::make_shared<UIPopupMenu>("entity_list_context_menu", factory.getStyle("popupMenu"), menuOptions);
	menu->setAnchor(UIAnchor(Vector2f(), Vector2f(), getRoot()->getLastMousePos()));
	getRoot()->addChild(menu);

	menu->setHandle(UIEventType::PopupAccept, [this, entityId] (const UIEvent& e) {
		onContextMenuAction(e.getStringData(), entityId);
	});
}

void EntityList::onContextMenuAction(const String& actionId, const String& entityId)
{
	sceneEditorWindow->onEntityContextMenuAction(actionId, entityId);
}
