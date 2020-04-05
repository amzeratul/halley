#include "world_scene_data.h"
using namespace Halley;

WorldSceneData::WorldSceneData(World& world)
	: world(world)
{
}

ConfigNode WorldSceneData::getEntityData(const String& id)
{
	// TODO
	return ConfigNode();
}

void WorldSceneData::reloadEntity(const String& id, const ConfigNode& data)
{
	// TODO
}

EntityTree WorldSceneData::getEntityTree() const
{
	// TODO
	return EntityTree();
}
