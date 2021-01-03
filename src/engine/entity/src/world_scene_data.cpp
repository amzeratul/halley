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
	throw Exception("Not implemented", HalleyExceptions::Entity);
}

EntityTree WorldSceneData::getEntityTree() const
{
	throw Exception("Not implemented", HalleyExceptions::Entity);
}

std::pair<String, int> WorldSceneData::reparentEntity(const String& entityId, const String& newParentId, int childIndex)
{
	throw Exception("Not implemented", HalleyExceptions::Entity);
}

bool WorldSceneData::isSingleRoot()
{
	return false;
}
