#include "prefab_scene_data.h"
using namespace Halley;

PrefabSceneData::PrefabSceneData(Prefab& prefab)
	: prefab(prefab)
{
}

ConfigNode PrefabSceneData::getEntityData(const String& id)
{
	return findEntity(prefab.getRoot(), id).value_or(ConfigNode());
}

Maybe<ConfigNode> PrefabSceneData::findEntity(ConfigNode& node, const String& id)
{
	if (node["uuid"].asString("") == id) {
		return ConfigNode(node);
	}
	
	if (node["children"].getType() == ConfigNodeType::Sequence) {
		for (auto& childNode : node["children"].asSequence()) {
			auto r = findEntity(childNode, id);
			if (r) {
				return r;
			}
		}
	}

	return {};
}
