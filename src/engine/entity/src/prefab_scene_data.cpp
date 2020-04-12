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

ISceneData::EntityData PrefabSceneData::getEntityData(const String& id)
{
	if (id.isEmpty()) {
		return EntityData(prefab.getRoot(), "");
	}
	
	const auto& [data, parentData] = findEntityAndParent(prefab.getRoot(), nullptr, id);
	if (!data) {
		throw Exception("Entity data not found for \"" + id + "\"", HalleyExceptions::Entity);
	}

	String parentId;
	if (parentData) {
		parentId = (*parentData)["uuid"].asString();
	}
	return EntityData(*data, parentId);
}

void PrefabSceneData::reloadEntity(const String& id)
{
	if (!id.isEmpty()) {
		reloadEntity(id, findEntity(prefab.getRoot(), id));
	}
}

void PrefabSceneData::reloadEntity(const String& id, ConfigNode* data)
{
	auto entity = world.findEntity(UUID(id));
	if (entity) {
		if (data) {
			// Update
			factory->updateEntityTree(*entity, *data);
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
	const auto& rootNode = prefab.getRoot();
	if (rootNode.getType() == ConfigNodeType::Sequence) {
		for (auto& n: rootNode.asSequence()) {
			fillEntityTree(n, root.children.emplace_back());
		}
	} else {
		fillEntityTree(rootNode, root);
	}
	return root;
}

void PrefabSceneData::fillEntityTree(const ConfigNode& node, EntityTree& tree) const
{
	tree.entityId = node["uuid"].asString("");
	if (node.hasKey("prefab")) {
		const auto& prefabName = node["prefab"].asString();
		const auto& prefabNode = gameResources.get<Prefab>(prefabName)->getRoot();
		//tree.entityId = prefabNode["uuid"].asString("");
		tree.name = prefabNode["name"].asString("");
		tree.prefab = prefabName;
	} else {
		tree.name = node["name"].asString("Entity");
		if (node["children"].getType() == ConfigNodeType::Sequence) {
			const auto& seq = node["children"].asSequence();
			tree.children.reserve(seq.size());

			for (auto& childNode : seq) {
				tree.children.emplace_back();
				fillEntityTree(childNode, tree.children.back());
			}
		}
	}
}

void PrefabSceneData::reparentEntity(const String& entityId, const String& newParentId, int childIndex)
{
	Expects(childIndex >= 0);
	Expects(!entityId.isEmpty());
	
	auto [entityMoving, oldParent] = findEntityAndParent(prefab.getRoot(), nullptr, entityId);

	if (!entityMoving) {
		throw Exception("Entity not found: " + entityId, HalleyExceptions::Tools);
	}
	if (!oldParent) {
		throw Exception("Entity has no parent: " + entityId, HalleyExceptions::Tools);
	}
	const String oldParentId = (*oldParent)["uuid"].asString();

	// WARNING: THESE OPERATIONS CAN INVALIDATE OLD POINTERS, DON'T KEEP REFERENCES
	if (newParentId == oldParentId) {
		const auto newParent = findEntity(prefab.getRoot(), newParentId);
		if (!newParent) {
			throw Exception("Entity not found: " + newParentId, HalleyExceptions::Tools);
		}
		moveChild(*newParent, entityId, size_t(childIndex)); // INVALIDATES REFERENCES
	} else {
		auto child = removeChild(*oldParent, entityId); // INVALIDATES REFERENCES
		const auto newParent = findEntity(prefab.getRoot(), newParentId);
		if (!newParent) {
			throw Exception("Entity not found: " + newParentId, HalleyExceptions::Tools);
		}
		addChild(*newParent, childIndex, std::move(child)); // INVALIDATES REFERENCES
	}

	
	reloadEntity(oldParentId, findEntity(prefab.getRoot(), oldParentId));
	reloadEntity(newParentId, findEntity(prefab.getRoot(), newParentId));
}

ConfigNode* PrefabSceneData::findEntity(ConfigNode& node, const String& id)
{
	if (node.getType() == ConfigNodeType::Sequence) {
		for (auto& childNode: node.asSequence()) {
			const auto r = findEntity(childNode, id);
			if (r) {
				return r;
			}
		}
	} else {
		if (node["uuid"].asString("") == id) {
			return &node;
		}

		if (node["children"].getType() == ConfigNodeType::Sequence) {
			for (auto& childNode : node["children"].asSequence()) {
				auto r = findEntity(childNode, id);
				if (r) {
					return r;
				}
			}
		}
	}
	
	return nullptr;
}

std::pair<ConfigNode*, ConfigNode*> PrefabSceneData::findEntityAndParent(ConfigNode& node, ConfigNode* previous, const String& id)
{
	if (node.getType() == ConfigNodeType::Sequence) {
		for (auto& childNode: node.asSequence()) {
			auto r = findEntityAndParent(childNode, nullptr, id);
			if (r.first) {
				return r;
			}
		}
	} else {
		if (node["uuid"].asString("") == id) {
			return std::make_pair(&node, previous);
		}

		if (node["children"].getType() == ConfigNodeType::Sequence) {
			for (auto& childNode : node["children"].asSequence()) {
				auto r = findEntityAndParent(childNode, &node, id);
				if (r.first) {
					return r;
				}
			}
		}
	}

	return std::make_pair(nullptr, nullptr);
}

void PrefabSceneData::addChild(ConfigNode& parent, int index, ConfigNode child)
{
	if (parent["children"].getType() != ConfigNodeType::Sequence) {
		parent["children"] = ConfigNode::SequenceType();
	}

	auto& seq = parent["children"].asSequence();
	seq.insert(seq.begin() + clamp(index, 0, int(seq.size())), std::move(child));
}

ConfigNode PrefabSceneData::removeChild(ConfigNode& parent, const String& childId)
{
	auto& seq = parent["children"].asSequence();
	for (size_t i = 0; i < seq.size(); ++i) {
		auto& node = seq[i].asMap();
		if (node["uuid"].asString("") == childId) {
			ConfigNode result = std::move(node);
			seq.erase(seq.begin() + i);
			return result;
		}
	}
	throw Exception("Child not found: " + childId, HalleyExceptions::Tools);
}

void PrefabSceneData::moveChild(ConfigNode& parent, const String& childId, int targetIndex)
{
	auto& seq = parent["children"].asSequence();
	const int startIndex = int(std::find_if(seq.begin(), seq.end(), [&] (const ConfigNode& n) { return n["uuid"].asString("") == childId; }) - seq.begin());

	// If moving forwards, subtract one to account for the fact that the currently occupied slot will be removed
	const int finalIndex = targetIndex > startIndex ? targetIndex - 1 : targetIndex;

	// Swap from start to end
	const int dir = signOf(finalIndex - startIndex);
	for (int i = startIndex; i != finalIndex; i += dir) {
		std::swap(seq[i], seq[i + dir]);
	}
}
