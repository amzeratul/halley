#include "prefab_scene_data.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/core/resources/resources.h"
#include "halley/support/logger.h"
#include "world.h"

#include "components/transform_2d_component.h"

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
		return EntityNodeData(prefab.getEntityData(), "", 0);
	}
	
	const auto& data = findEntityAndParent(prefab.getEntityDatas(), nullptr, 0, id);
	if (!data.entity) {
		throw Exception("Entity data not found for \"" + id + "\"", HalleyExceptions::Entity);
	}

	String parentId;
	if (data.parent) {
		parentId = data.parent->getInstanceUUID().toString();
	}
	return EntityNodeData(*data.entity, parentId, static_cast<int>(data.childIdx));
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
			factory->updateEntity(*entity, *data, static_cast<int>(EntitySerialization::Type::Prefab));
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
	tree.data = &node;

	if (node.getPrefab().isEmpty()) {
		const auto& seq = node.getChildren();
		tree.children.reserve(seq.size());
		for (const auto& childNode : seq) {
			tree.children.emplace_back();
			fillEntityTree(childNode, tree.children.back());
		}
	}
}

std::pair<String, size_t> PrefabSceneData::reparentEntity(const String& entityId, const String& newParentId, size_t childIndex)
{
	Expects(childIndex >= 0);
	Expects(!entityId.isEmpty());
	
	const auto data = findEntityAndParent(prefab.getEntityDatas(), nullptr, 0, entityId);
	const auto oldParent = data.parent;
	const auto oldChildIndex = data.childIdx;

	if (!data.entity) {
		throw Exception("Entity not found: " + entityId, HalleyExceptions::Tools);
	}
	const String oldParentId = oldParent ? (*oldParent).getInstanceUUID().toString() : "";

	// WARNING: ALL OF THESE OPERATIONS CAN INVALIDATE OLD POINTERS, DON'T KEEP REFERENCES
	if (newParentId == oldParentId) {
		moveChild(findEntity(newParentId), entityId, static_cast<int>(childIndex)); // INVALIDATES REFERENCES
		reloadEntity(newParentId.isEmpty() ? entityId : newParentId);
	} else {
		// Updates the transform to retain world position
		const auto newParentEntity = newParentId.isEmpty() ? std::optional<EntityRef>() : world.findEntity(UUID(newParentId));
		const auto entity = world.findEntity(UUID(entityId));
		makeTransformRelative(entity.value(), newParentEntity, *data.entity);
		
		// The order is very important here
		// Don't collapse into one sequence point! findEntity(newParentId) MUST execute after removeChild()!
		auto child = removeChild(findEntity(oldParentId), entityId); // INVALIDATES REFERENCES

		// Reload before proceeding, so it can delete from root if needed
		reloadEntity(oldParentId.isEmpty() ? entityId : oldParentId);

		// Add to new parent
		addChild(findEntity(newParentId), static_cast<int>(childIndex), std::move(child)); // INVALIDATES REFERENCES

		// Reload destination
		reloadEntity(newParentId.isEmpty() ? entityId : newParentId);
	}

	return { oldParentId, oldChildIndex };
}

std::pair<String, size_t> PrefabSceneData::getEntityParenting(const String& entityId)
{
	const auto data = findEntityAndParent(prefab.getEntityDatas(), nullptr, 0, entityId);
	const auto oldParent = data.parent;
	const auto oldChildIndex = data.childIdx;

	if (!data.entity) {
		throw Exception("Entity not found: " + entityId, HalleyExceptions::Tools);
	}
	const String oldParentId = oldParent ? (*oldParent).getInstanceUUID().toString() : "";
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
	for (size_t i = 0; i < node.size(); ++i) {
		const auto r = findEntityAndParent(node[i], nullptr, i, id);
		if (r.entity) {
			return r.entity;
		}
	}

	return nullptr;
}

PrefabSceneData::EntityAndParent PrefabSceneData::findEntityAndParent(gsl::span<EntityData> node, EntityData* previous, size_t idx, const String& id)
{
	for (size_t i = 0; i < node.size(); ++i) {
		const auto r = findEntityAndParent(node[i], nullptr, i, id);
		if (r.entity) {
			return r;
		}
	}

	return {};
}

PrefabSceneData::EntityAndParent PrefabSceneData::findEntityAndParent(EntityData& node, EntityData* previous, size_t idx, const String& id)
{
	if (node.getInstanceUUID().toString() == id) {
		return { &node, previous, idx };
	}

	auto& children = node.getChildren();
	for (size_t i = 0; i < children.size(); ++i) {
		const auto r = findEntityAndParent(children[i], &node, i, id);
		if (r.entity) {
			return r;
		}
	}

	return {};
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
	
	// Swap from start to end
	const int dir = signOf(targetIndex - startIndex);
	for (int i = startIndex; i != targetIndex; i += dir) {
		std::swap(seq[i], seq[i + dir]);
	}
}

void PrefabSceneData::makeTransformRelative(EntityRef entity, std::optional<EntityRef> newParent, EntityData& entityData)
{
	if (entity.hasComponent<Transform2DComponent>()) {
		makeTransformRelative2D(entity, newParent, entityData);
	}
}

void PrefabSceneData::makeTransformRelative2D(EntityRef entity, std::optional<EntityRef> newParent, EntityData& entityData)
{
	const auto& transform = entity.getComponent<Transform2DComponent>();

	// TODO: use matrix operations to do this properly
	const auto pos = transform.getGlobalPosition();
	Vector2f localPos = pos;

	if (newParent && newParent->hasComponent<Transform2DComponent>()) {
		const auto& parentTransform = newParent->getComponent<Transform2DComponent>();
		localPos = parentTransform.inverseTransformPoint(pos);
	}

	// Apply to data
	for (auto& c: entityData.getComponents()) {
		if (c.first == "Transform2D") {
			c.second["position"] = ConfigNode(localPos);
		}
	}
}
