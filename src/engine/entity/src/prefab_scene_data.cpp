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

ISceneData::EntityNodeData PrefabSceneData::getEntityNodeData(const String& id)
{
	if (id.isEmpty()) {
		return EntityNodeData(prefab.getEntityNodeRoot(), "");
	}
	
	const auto& [data, parentData] = findEntityAndParent(prefab.getEntityNodeRoot(), nullptr, id);
	if (!data) {
		throw Exception("Entity data not found for \"" + id + "\"", HalleyExceptions::Entity);
	}

	String parentId;
	if (parentData) {
		parentId = (*parentData)["uuid"].asString();
	}
	return EntityNodeData(*data, parentId);
}

void PrefabSceneData::reloadEntity(const String& id)
{
	if (!id.isEmpty()) {
		reloadEntity(id, findEntity(prefab.getEntityNodeRoot(), id));
		world.spawnPending();
	}
}

void PrefabSceneData::reloadEntity(const String& id, ConfigNode* data)
{
	auto entity = world.findEntity(UUID(id));
	if (entity) {
		if (data) {
			// Update
			factory->updateEntity(*entity, EntityData(*data, false));
		} else {
			// Destroy
			world.destroyEntity(entity.value());
		}
	} else {
		if (data) {
			// Create
			factory->createEntity(EntityData(*data, false));
		}
	}
}

EntityTree PrefabSceneData::getEntityTree() const
{
	EntityTree root;
	const auto& rootNode = prefab.getEntityNodeRoot();
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
		tree.prefab = prefabName;
		if (gameResources.exists<Prefab>(prefabName)) {
			tree.name = gameResources.get<Prefab>(prefabName)->getPrefabName();
		} else {
			tree.name = "Missing Prefab";
		}
	} else {
		tree.name = node["name"].asString("Entity");
		if (node["children"].getType() == ConfigNodeType::Sequence) {
			const auto& seq = node["children"].asSequence();
			tree.children.reserve(seq.size());

			for (const auto& childNode : seq) {
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
	
	auto [entityMoving, oldParent] = findEntityAndParent(prefab.getEntityNodeRoot(), nullptr, entityId);

	if (!entityMoving) {
		throw Exception("Entity not found: " + entityId, HalleyExceptions::Tools);
	}
	const String oldParentId = oldParent ? (*oldParent)["uuid"].asString() : "";

	// WARNING: ALL OF THESE OPERATIONS CAN INVALIDATE OLD POINTERS, DON'T KEEP REFERENCES
	if (newParentId == oldParentId) {
		moveChild(findChildListFor(newParentId), entityId, size_t(childIndex)); // INVALIDATES REFERENCES
		reloadEntity(newParentId.isEmpty() ? entityId : newParentId);
	} else {
		// The order is very important here
		// Don't collapse into one sequence point! findChildListFor(newParentId) MUST execute after removeChild()!
		auto child = removeChild(findChildListFor(oldParentId), entityId); // INVALIDATES REFERENCES

		// Reload before proceeding, so it can delete from root if needed
		reloadEntity(oldParentId.isEmpty() ? entityId : oldParentId);

		// Add to new parent
		addChild(findChildListFor(newParentId), childIndex, std::move(child)); // INVALIDATES REFERENCES

		// Reload destination
		reloadEntity(newParentId.isEmpty() ? entityId : newParentId);
	}
}

bool PrefabSceneData::isSingleRoot()
{
	return !prefab.isScene();
}

ConfigNode::SequenceType& PrefabSceneData::findChildListFor(const String& id)
{
	auto& root = prefab.getEntityNodeRoot();
	if (id.isEmpty()) {
		if (root.getType() == ConfigNodeType::Sequence) {
			return root.asSequence();
		} else {
			throw Exception("Root has no child list", HalleyExceptions::Entity);
		}		
	} else {
		ConfigNode* res = nullptr;

		if (root.getType() == ConfigNodeType::Sequence) {
			for (auto& r: root.asSequence()) {
				res = doFindChildListFor(r, id);
				if (res) {
					break;
				}
			}
		} else {
			res = doFindChildListFor(root, id);
		}

		if (!res) {
			throw Exception("Couldn't find entity with id " + id, HalleyExceptions::Entity);
		}
		ConfigNode& children = *res;
		if (children.getType() == ConfigNodeType::Undefined) {
			children = ConfigNode::SequenceType();
		}
		return children.asSequence();
	}
}

ConfigNode* PrefabSceneData::doFindChildListFor(ConfigNode& node, const String& id)
{
	if (node["uuid"].asString("") == id) {
		return &node["children"];
	}

	if (node["children"].getType() == ConfigNodeType::Sequence) {
		for (auto& childNode : node["children"].asSequence()) {
			const auto r = doFindChildListFor(childNode, id);
			if (r) {
				return r;
			}
		}
	}

	return nullptr;
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

void PrefabSceneData::addChild(ConfigNode::SequenceType& seq, int index, ConfigNode child)
{
	seq.insert(seq.begin() + clamp(index, 0, int(seq.size())), std::move(child));
}

ConfigNode PrefabSceneData::removeChild(ConfigNode::SequenceType& seq, const String& childId)
{
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

void PrefabSceneData::moveChild(ConfigNode::SequenceType& seq, const String& childId, int targetIndex)
{
	const int startIndex = int(std::find_if(seq.begin(), seq.end(), [&] (const ConfigNode& n) { return n["uuid"].asString("") == childId; }) - seq.begin());

	// If moving forwards, subtract one to account for the fact that the currently occupied slot will be removed
	const int finalIndex = targetIndex > startIndex ? targetIndex - 1 : targetIndex;

	// Swap from start to end
	const int dir = signOf(finalIndex - startIndex);
	for (int i = startIndex; i != finalIndex; i += dir) {
		std::swap(seq[i], seq[i + dir]);
	}
}
