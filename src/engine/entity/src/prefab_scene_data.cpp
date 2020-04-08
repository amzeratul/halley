#include "prefab_scene_data.h"

#include "../../../tools/tools/include/halley/tools/file/filesystem.h"
#include "halley/bytes/byte_serializer.h"

using namespace Halley;

PrefabSceneData::PrefabSceneData(Prefab& prefab, std::shared_ptr<EntityFactory> factory, EntityRef entity)
	: prefab(prefab)
	, factory(std::move(factory))
	, entity(entity)
{
}

ConfigNode PrefabSceneData::getEntityData(const String& id)
{
	auto result = findEntity(prefab.getRoot(), id);
	if (result) {
		return ConfigNode(*result);
	}
	return ConfigNode();
}

void PrefabSceneData::reloadEntity(const String& id, const ConfigNode& data)
{
	auto result = findEntity(prefab.getRoot(), id);
	if (result) {
		*result = ConfigNode(data);
	}
	
	factory->updateEntityTree(entity, prefab.getRoot());
}

ConfigNode* PrefabSceneData::findEntity(ConfigNode& node, const String& id) const
{
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

	return nullptr;
}

EntityTree PrefabSceneData::getEntityTree() const
{
	EntityTree root;
	fillEntityTree(prefab.getRoot(), root);
	return root;
}

void PrefabSceneData::reparentEntity(const String& entityId, const String& newParentId, int childIndex)
{
	// TODO
}

void PrefabSceneData::fillEntityTree(const ConfigNode& node, EntityTree& tree) const
{
	tree.entityId = node["uuid"].asString("");
	tree.name = node["name"].asString("");
	if (node["children"].getType() == ConfigNodeType::Sequence) {
		const auto& seq = node["children"].asSequence();
		tree.children.reserve(seq.size());
		
		for (auto& childNode : seq) {
			tree.children.emplace_back();
			fillEntityTree(childNode, tree.children.back());
		}
	}
}
