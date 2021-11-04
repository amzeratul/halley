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

void EntityList::update(Time t, bool moved)
{
	if (needsToNotifyValidatorList) {
		needsToNotifyValidatorList = false;
		notifyValidatorList();
	}

	if (validationTimeout >= 0) {
		validationTimeout -= t;
	} else if (needsToValidateAllEntities) {
		validationTimeout = 0.4;
		needsToValidateAllEntities = false;
		doValidateAllEntities();
	}
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
	list->setMultiSelect(true);
	//list->setDragOutsideEnabled(true);
	add(list, 1);

	setHandle(UIEventType::TreeItemReparented, [=] (const UIEvent& event)
	{
		const auto& src = event.getConfigData().asSequence();
		std::vector<EntityChangeOperation> changes;
		changes.reserve(src.size());
		for (const auto& e: src) {
			changes.emplace_back(EntityChangeOperation{ {}, e["itemId"].asString(), e["parentId"].asString(), e["childIdx"].asInt() });
		}

		sceneEditorWindow->moveEntities(changes, false);
		notifyValidatorList();
	});

	setHandle(UIEventType::ListItemRightClicked, [=] (const UIEvent& event)
	{
		openContextMenu(list->getSelectedOptionIds());
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
	const auto info = getEntityInfo(data);
	const size_t idx = childIndex >= 0 ? static_cast<size_t>(childIndex) : std::numeric_limits<size_t>::max();
	list->addTreeItem(data.getInstanceUUID().toString(), parentId, idx, LocalisedString::fromUserString(info.name), isPrefab ? "labelSpecial" : "label", info.icon, isPrefab);
	markValid(data.getInstanceUUID(), info.valid);
}

void EntityList::addEntityTree(const String& parentId, int childIndex, const EntityData& data)
{
	const auto& curId = data.getInstanceUUID().toString();
	addEntity(data, parentId, childIndex);
	for (const auto& child: data.getChildren()) {
		addEntityTree(curId, -1, child);
	}
}

EntityList::EntityInfo EntityList::getEntityInfo(const EntityData& data) const
{
	EntityInfo result;

	if (data.getPrefab().isEmpty()) {
		result.name = data.getName().isEmpty() ? String("Unnamed Entity") : data.getName();
		result.icon = icons->getIcon(data.getIcon());
		result.valid = isEntityValid(data, false);
	} else {
		if (const auto prefab = sceneEditorWindow->getGamePrefab(data.getPrefab())) {
			result.name = prefab->getPrefabName();
			result.icon = icons->getIcon(prefab->getPrefabIcon());
			result.valid = isEntityValid(prefab->getEntityData().instantiateWithAsCopy(data), true);
		} else {
			result.name = "Missing prefab! [" + data.getPrefab() + "]";
			result.icon = icons->getIcon("");
			result.valid = false;
		}
	}

	if (!result.valid) {
		result.icon = icons->getInvalidEntityIcon();
	}

	return result;
}

void EntityList::refreshList()
{
	const auto prevId = list->getSelectedOptionId();

	markAllValid();

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
	onEntityModified(id, node, false);
}

void EntityList::onEntityModified(const String& id, const EntityData& node, bool onlyRefreshValidation)
{
	const auto info = getEntityInfo(node);
	const bool validationChanged = markValid(node.getInstanceUUID(), info.valid);
	if (validationChanged || !onlyRefreshValidation) {
		list->setLabel(id, LocalisedString::fromUserString(info.name), std::move(info.icon));
	}
}

void EntityList::onEntitiesAdded(gsl::span<const EntityChangeOperation> changes)
{
	std::vector<String> ids;

	for (const auto& change: changes) {
		addEntityTree(change.parent, change.childIndex, change.data->asEntityData());
		ids.push_back(change.entityId);
	}

	list->sortItems();
	layout();
	list->setSelectedOptionIds(ids);

	notifyValidatorList();
}

void EntityList::onEntitiesRemoved(gsl::span<const String> ids, const String& newSelectionId)
{
	for (const auto& id: ids) {
		list->removeItem(id);
		markValid(UUID(id), true);
	}
	list->sortItems();
	layout();
	list->setScrollToSelection(false);
	list->setSelectedOption(-1);
	list->setScrollToSelection(true);
	list->setSelectedOptionId(newSelectionId);
}

void EntityList::select(const String& id, UIList::SelectionMode mode)
{
	list->setSelectedOptionId(id, mode);
}

void EntityList::select(gsl::span<const String> ids, UIList::SelectionMode mode)
{
	list->setSelectedOptionIds(ids, mode);
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

std::vector<String> EntityList::getCurrentSelections() const
{
	return list->getSelectedOptionIds();

	/*
	// Ensures the active selection is always the first one 
	auto first = list->getSelectedOptionId();
	auto all = list->getSelectedOptionIds();
	std_ex::erase_if(all, [&] (const auto& v) { return v == first; });
	std::vector<String> result;
	result.reserve(1 + all.size());
	result.push_back(first);
	result.insert(result.end(), all.begin(), all.end());
	return result;
	*/
}

void EntityList::setEntityValidatorList(std::shared_ptr<EntityValidatorListUI> validator)
{
	validatorList = std::move(validator);
}

UITreeList& EntityList::getList()
{
	return *list;
}

void EntityList::openContextMenu(gsl::span<const String> entityIds)
{
	auto menuOptions = std::vector<UIPopupMenuItem>();
	auto makeEntry = [&] (const String& id, const String& text, const String& toolTip, const String& icon, bool enabled = true)
	{
		auto iconSprite = Sprite().setImage(factory.getResources(), "entity_icons/" + (icon.isEmpty() ? "empty.png" : icon));
		menuOptions.push_back(UIPopupMenuItem(id, LocalisedString::fromHardcodedString(text), std::move(iconSprite), LocalisedString::fromHardcodedString(toolTip)));
		menuOptions.back().enabled = enabled;
	};

	const bool canPaste = sceneEditorWindow->canPasteEntity();
	const bool canAddAsSibling = !entityIds.empty() && sceneEditorWindow->canAddSibling(entityIds.front());
	const bool isPrefab = !entityIds.empty() && sceneEditorWindow->isPrefabInstance(entityIds.front());
	const bool canExtractPrefab = canAddAsSibling;
	const bool canAddAsChild = !isPrefab;
	const bool canRemove = canAddAsSibling;
	const bool isSingle = entityIds.size() == 1;

	if (isSingle) {
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
		makeEntry("paste_sibling", "Paste", "Paste entities as a sibling of the current one. [Ctrl+V]", "paste.png", canPaste && canAddAsSibling);
		makeEntry("paste_child", "Paste (Child)", "Paste entity as a child of the current one.", "", canPaste && canAddAsChild);
		menuOptions.emplace_back();
		makeEntry("duplicate", "Duplicate", "Duplicate entity [Ctrl+D]", "", canAddAsSibling);
		makeEntry("delete", "Delete", "Delete entity [Del]", "delete.png", canRemove);
	} else {
		makeEntry("cut", "Cut", "Cut entities to clipboard [Ctrl+X]", "cut.png", canRemove);
		makeEntry("copy", "Copy", "Copy entities to clipboard [Ctrl+C]", "copy.png");
		menuOptions.emplace_back();
		makeEntry("duplicate", "Duplicate", "Duplicate entities [Ctrl+D]", "", canAddAsSibling);
		makeEntry("delete", "Delete", "Delete entities [Del]", "delete.png", canRemove);
	}
	
	auto menu = std::make_shared<UIPopupMenu>("entity_list_context_menu", factory.getStyle("popupMenu"), menuOptions);
	menu->spawnOnRoot(*getRoot());

	menu->setHandle(UIEventType::PopupAccept, [this, entityIds] (const UIEvent& e) {
		Concurrent::execute(Executors::getMainThread(), [=] () {
			onContextMenuAction(e.getStringData(), entityIds);
		});
	});
}

void EntityList::onContextMenuAction(const String& actionId, gsl::span<const String> entityIds)
{
	sceneEditorWindow->onEntityContextMenuAction(actionId, entityIds);
}

bool EntityList::markAllValid()
{
	if (!invalidEntities.empty()) {
		invalidEntities.clear();
		notifyValidatorList();
		return true;
	}
	return false;
}

bool EntityList::markValid(const UUID& uuid, bool valid)
{
	bool modified = false;
	if (valid) {
		modified = invalidEntities.erase(uuid) > 0;
	} else {
		auto [iter, inserted] = invalidEntities.insert(uuid);
		modified = inserted;
	}

	if (modified) {
		needsToNotifyValidatorList = true;
	}
	return modified;
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

void EntityList::validateAllEntities()
{
	needsToValidateAllEntities = true;
}

void EntityList::doValidateAllEntities()
{
	const auto& sceneData = sceneEditorWindow->getSceneData();
	
	const auto n = list->getCount();
	for (size_t i = 0; i < n; ++i) {
		const auto& id = list->getItem(static_cast<int>(i))->getId();
		const auto& entityData = sceneData->getEntityNodeData(id);
		onEntityModified(id, entityData.getData(), true);
	}
}

bool EntityList::isEntityValid(const EntityData& entityData, bool recursive) const
{
	return sceneEditorWindow->getEntityValidator().validateEntity(entityData, recursive).empty();
}
