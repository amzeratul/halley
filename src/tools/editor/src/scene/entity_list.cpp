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

		sceneEditorWindow->moveEntity(entityId, newParentId, childIndex, false);
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
		addEntity(*entity.data, parentId, -1);
	}

	for (auto& e: entity.children) {
		addEntities(e, entity.entityId);
	}
}

void EntityList::addEntity(const EntityData& data, const String& parentId, int childIndex)
{
	const bool isPrefab = !data.getPrefab().isEmpty();
	const auto info = getEntityNameAndIcon(data);
	const size_t idx = childIndex >= 0 ? static_cast<size_t>(childIndex) : std::numeric_limits<size_t>::max();
	list->addTreeItem(data.getInstanceUUID().toString(), parentId, idx, LocalisedString::fromUserString(info.name), isPrefab ? "labelSpecial" : "label", info.icon, isPrefab);

	if (!info.valid) {
		invalidEntities.insert(data.getInstanceUUID());
		notifyValidatorList();
	}
}

EntityList::EntityInfo EntityList::getEntityNameAndIcon(const EntityData& data) const
{
	EntityInfo result;

	if (!data.getPrefab().isEmpty()) {
		if (const auto prefab = sceneEditorWindow->getGamePrefab(data.getPrefab())) {
			result.name = prefab->getPrefabName();
			result.icon = icons->getIcon(prefab->getPrefabIcon());
		} else {
			result.name = "Missing prefab! [" + data.getPrefab() + "]";
			result.icon = icons->getIcon("");
		}
	} else {
		result.name = data.getName().isEmpty() ? String("Unnamed Entity") : data.getName();
		result.icon = icons->getIcon(data.getIcon());
	}

	const auto validateResult = sceneEditorWindow->getEntityValidator().validateEntity(data);
	if (!validateResult.empty()) {
		result.icon = icons->getInvalidEntityIcon();
		result.valid = false;
	}

	return result;
}

void EntityList::refreshList()
{
	const auto prevId = list->getSelectedOptionId();

	list->setScrollToSelection(false);
	list->clear();
	invalidEntities.clear();
	if (sceneData) {
		addEntities(sceneData->getEntityTree(), "");
	}
	layout();
	list->setScrollToSelection(true);

	list->setSelectedOptionId(prevId);

	notifyValidatorList();
}

void EntityList::refreshNames()
{
	refreshList();
}

void EntityList::onEntityModified(const String& id, const EntityData& node)
{
	const auto info = getEntityNameAndIcon(node);
	list->setLabel(id, LocalisedString::fromUserString(info.name), info.icon);

	if (info.valid) {
		invalidEntities.erase(node.getInstanceUUID());
	} else {
		invalidEntities.insert(node.getInstanceUUID());
	}
	notifyValidatorList();
}

void EntityList::onEntityAdded(const String& id, const String& parentId, int childIndex, const EntityData& data)
{
	addEntityTree(parentId, childIndex, data);
	list->sortItems();
	layout();
	list->setSelectedOptionId(id);

	notifyValidatorList();
}

void EntityList::addEntityTree(const String& parentId, int childIndex, const EntityData& data)
{
	const auto& curId = data.getInstanceUUID().toString();
	addEntity(data, parentId, childIndex);
	for (const auto& child: data.getChildren()) {
		addEntityTree(curId, -1, child);
	}
}

void EntityList::onEntityRemoved(const String& id, const String& newSelectionId)
{
	list->removeItem(id);
	list->sortItems();
	layout();
	list->setScrollToSelection(false);
	list->setSelectedOption(-1);
	list->setScrollToSelection(true);
	list->setSelectedOptionId(newSelectionId);

	invalidEntities.erase(UUID(id));
	notifyValidatorList();
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

void EntityList::setEntityValidatorList(std::shared_ptr<EntityValidatorListUI> validator)
{
	validatorList = std::move(validator);
}

UITreeList& EntityList::getList()
{
	return *list;
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

	const bool canPaste = sceneEditorWindow->canPasteEntity();
	const bool canAddAsSibling = sceneEditorWindow->canAddSibling(entityId);
	const bool isPrefab = sceneEditorWindow->isPrefabInstance(entityId);
	const bool canExtractPrefab = canAddAsSibling;
	const bool canAddAsChild = !isPrefab;
	const bool canRemove = canAddAsSibling;
	
	makeEntry("add_entity_sibling", "Add Entity", "Adds an empty entity as a sibling of this one.", "", canAddAsSibling);
	makeEntry("add_entity_child", "Add Entity (Child)", "Adds an empty entity as a child of this one.", "", canAddAsChild);
	makeEntry("add_prefab_sibling", "Add Prefab", "Adds a prefab as a sibling of this entity.", "", canAddAsSibling);
	makeEntry("add_prefab_child", "Add Prefab (Child)", "Adds a prefab as a child of this entity.", "", canAddAsChild);
	menuOptions.emplace_back();
	if (isPrefab) {
		makeEntry("collapse_prefab", "Collapse Prefab", "Imports the current prefab directly into the scene.", "");
	} else {
		makeEntry("extract_prefab", "Extract Prefab...", "Converts the current entity into a new prefab.", "", canExtractPrefab);
	}
	menuOptions.emplace_back();
	makeEntry("cut", "Cut", "Cut entity to clipboard [Ctrl+X]", "cut.png", canRemove);
	makeEntry("copy", "Copy", "Copy entity to clipboard [Ctrl+C]", "copy.png");
	makeEntry("paste_sibling", "Paste", "Paste entity as a sibling of the current one. [Ctrl+V]", "paste.png", canPaste && canAddAsSibling);
	makeEntry("paste_child", "Paste (Child)", "Paste entity as a child of the current one.", "", canPaste && canAddAsChild);
	menuOptions.emplace_back();
	makeEntry("duplicate", "Duplicate", "Duplicate entity [Ctrl+D]", "", canAddAsSibling);
	makeEntry("delete", "Delete", "Delete entity [Del]", "delete.png", canRemove);
	
	auto menu = std::make_shared<UIPopupMenu>("entity_list_context_menu", factory.getStyle("popupMenu"), menuOptions);
	menu->spawnOnRoot(*getRoot());

	menu->setHandle(UIEventType::PopupAccept, [this, entityId] (const UIEvent& e) {\
		Concurrent::execute(Executors::getMainThread(), [=] () {
			onContextMenuAction(e.getStringData(), entityId);
		});
	});
}

void EntityList::onContextMenuAction(const String& actionId, const String& entityId)
{
	sceneEditorWindow->onEntityContextMenuAction(actionId, entityId);
}

void EntityList::notifyValidatorList()
{
	std::vector<int> result;
	result.reserve(invalidEntities.size());

	const auto n = list->getCount();
	for (size_t i = 0; i < n; ++i) {
		const auto& id = list->getItem(static_cast<int>(i))->getId();
		if (std_ex::contains(invalidEntities, UUID(id))) {
			result.push_back(static_cast<int>(i));
		}
	}
	validatorList->setInvalidEntities(std::move(result));
}
