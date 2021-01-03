#include "world_scene_data.h"
using namespace Halley;

WorldSceneData::WorldSceneData(World& world)
	: world(world)
{
}

ISceneData::EntityNodeData WorldSceneData::getWriteableEntityNodeData(const String& id)
{
	throw Exception("Not implemented", HalleyExceptions::Entity);
}

ISceneData::ConstEntityNodeData WorldSceneData::getEntityNodeData(const String& id)
{
	throw Exception("Not implemented", HalleyExceptions::Entity);
}

void WorldSceneData::reloadEntity(const String& id)
{
	// TODO
}

EntityTree WorldSceneData::getEntityTree() const
{
	// TODO
	return EntityTree();
}

void WorldSceneData::reparentEntity(const String& entityId, const String& newParentId, int childIndex)
{
	// TODO
}

bool WorldSceneData::isSingleRoot()
{
	return false;
}
