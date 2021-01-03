#include "prefab_scene_data.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/core/resources/resources.h"
#include "halley/support/logger.h"
#include "world.h"

using namespace Halley;

PrefabSceneData::PrefabSceneData(Prefab& prefab, std::shared_ptr<EntityFactory> factory, World& world, Resources& gameResources)
	: prefab(prefab)
	, factory(std::move(factory))
	, world(world)
	, gameResources(gameResources)
{
}

ISceneData::EntityNodeData PrefabSceneData::getWriteableEntityNodeData(const String& id)
{
	if (id.isEmpty()) {
		return EntityNodeData(prefab.getEntityData(), "");
	}
	
	const auto& [data, parentData] = findEntityAndParent(prefab.getEntityDatas(), nullptr, id);
	if (!data) {
		throw Exception("Entity data not found for \"" + id + "\"", HalleyExceptions::Entity);
	}

	String parentId;
	if (parentData) {
		parentId = (*parentData).getInstanceUUID().toString();
	}
	return EntityNodeData(*data, parentId);
}

ISceneData::ConstEntityNodeData PrefabSceneData::getEntityNodeData(const String& id)
{
	return ConstEntityNodeData(getWriteableEntityNodeData(id));
}

void PrefabSceneData::reloadEntity(const String& id)
{
	if (!id.isEmpty()) {
		reloadEntity(id, findEntity(prefab.getEntityDatas(), id));
		world.spawnPending();
	}
}

void PrefabSceneData::reloadEntity(const String& id, EntityData* data)
{
	auto entity = world.findEntity(UUID(id));
	if (entity) {
		if (data) {
			// Update
			factory->updateEntity(*entity, *data);
		} else {
			// Destroy
			world.destroyEntity(entity.value());
		}
	} else {
		if (data) {
			// Create
			factory->createEntity(*data);
		}
	}
}

EntityTree PrefabSceneData::getEntityTree() const
{
	EntityTree root;
	for (auto& n: prefab.getEntityDatas()) {
		fillEntityTree(n, root.children.emplace_back());
	}
	return root;
}

void PrefabSceneData::fillEntityTree(const EntityData& node, EntityTree& tree) const
{
	tree.entityId = node.getInstanceUUID().toString();
	if (!node.getPrefab().isEmpty()) {
		const auto& prefabName = node.getPrefab();
		tree.prefab = prefabName;
		if (gameResources.exists<Prefab>(prefabName)) {
			const auto prefab = gameResources.get<Prefab>(prefabName);
			tree.name = prefab->getPrefabName();
			tree.icon = prefab->getPrefabIcon();
		} else {
			tree.name = "Missing Prefab";
			tree.icon = "";
		}
	} else {
		tree.name = node.getName();
		tree.icon = node.getIcon();
		if (tree.name.isEmpty()) {
			tree.name = "Entity";
		}
		
		const auto& seq = node.getChildren();
		tree.children.reserve(seq.size());
		for (const auto& childNode : seq) {
			tree.children.emplace_back();
			fillEntityTree(childNode, tree.children.back());
		}
	}
}

std::pair<String, int> PrefabSceneData::reparentEntity(const String& entityId, const String& newParentId, int childIndex)
{
	Expects(childIndex >= 0);
	Expects(!entityId.isEmpty());
	
	auto [entityMoving, oldParent] = findEntityAndParent(prefab.getEntityDatas(), nullptr, entityId);

	if (!entityMoving) {
		throw Exception("Entity not found: " + entityId, HalleyExceptions::Tools);
	}
	const String oldParentId = oldParent ? (*oldParent).getInstanceUUID().toString() : "";
	const int oldChildIndex = oldParent ? static_cast<int>(oldParent->getChildIndex(UUID(entityId)).value_or(0)) : -1;

	// WARNING: ALL OF THESE OPERATIONS CAN INVALIDATE OLD POINTERS, DON'T KEEP REFERENCES
	if (newParentId == oldParentId) {
		moveChild(findEntity(newParentId), entityId, size_t(childIndex)); // INVALIDATES REFERENCES
		reloadEntity(newParentId.isEmpty() ? entityId : newParentId);
	} else {
		// The order is very important here
		// Don't collapse into one sequence point! findEntity(newParentId) MUST execute after removeChild()!
		auto child = removeChild(findEntity(oldParentId), entityId); // INVALIDATES REFERENCES

		// Reload before proceeding, so it can delete from root if needed
		reloadEntity(oldParentId.isEmpty() ? entityId : oldParentId);

		// Add to new parent
		addChild(findEntity(newParentId), childIndex, std::move(child)); // INVALIDATES REFERENCES

		// Reload destination
		reloadEntity(newParentId.isEmpty() ? entityId : newParentId);
	}

	return { oldParentId, oldChildIndex };
}

bool PrefabSceneData::isSingleRoot()
{
	return !prefab.isScene();
}

EntityData& PrefabSceneData::findEntity(const String& id)
{
	auto* data = prefab.findEntityData(id.isEmpty() ? UUID() : UUID(id));
	if (!data) {
		throw Exception("Couldn't find entity with id " + id, HalleyExceptions::Entity);
	}
	return *data;
}

EntityData* PrefabSceneData::findEntity(gsl::span<EntityData> node, const String& id)
{
	for (auto& childNode: node) {
		const auto r = findEntityAndParent(childNode, nullptr, id);
		if (r.first) {
			return r.first;
		}
	}

	return nullptr;
}

std::pair<EntityData*, EntityData*> PrefabSceneData::findEntityAndParent(gsl::span<EntityData> node, EntityData* previous, const String& id)
{
	for (auto& childNode: node) {
		auto r = findEntityAndParent(childNode, nullptr, id);
		if (r.first) {
			return r;
		}
	}

	return std::make_pair(nullptr, nullptr);
}

std::pair<EntityData*, EntityData*> PrefabSceneData::findEntityAndParent(EntityData& node, EntityData* previous, const String& id)
{
	if (node.getInstanceUUID().toString() == id) {
		return std::make_pair(&node, previous);
	}

	for (auto& childNode : node.getChildren()) {
		auto r = findEntityAndParent(childNode, &node, id);
		if (r.first) {
			return r;
		}
	}

	return std::make_pair(nullptr, nullptr);
}

void PrefabSceneData::addChild(EntityData& parent, int index, EntityData child)
{
	auto& seq = parent.getChildren();
	seq.insert(seq.begin() + clamp(index, 0, int(seq.size())), std::move(child));
}

EntityData PrefabSceneData::removeChild(EntityData& parent, const String& childId)
{
	auto& seq = parent.getChildren();
	for (size_t i = 0; i < seq.size(); ++i) {
		auto& node = seq[i];
		if (node.getInstanceUUID().toString() == childId) {
			EntityData result = std::move(node);
			seq.erase(seq.begin() + i);
			return result;
		}
	}
	throw Exception("Child not found: " + childId, HalleyExceptions::Tools);
}

void PrefabSceneData::moveChild(EntityData& parent, const String& childId, int targetIndex)
{
	auto& seq = parent.getChildren();
	const int startIndex = int(std::find_if(seq.begin(), seq.end(), [&] (const EntityData& n) { return n.getInstanceUUID().toString() == childId; }) - seq.begin());

	// If moving forwards, subtract one to account for the fact that the currently occupied slot will be removed
	const int finalIndex = targetIndex > startIndex ? targetIndex - 1 : targetIndex;

	// Swap from start to end
	const int dir = signOf(finalIndex - startIndex);
	for (int i = startIndex; i != finalIndex; i += dir) {
		std::swap(seq[i], seq[i + dir]);
	}
}
