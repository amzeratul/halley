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

std::vector<EntityData*> WorldSceneData::getWriteableEntityDatas(gsl::span<const String> strings)
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

std::pair<String, size_t> WorldSceneData::reparentEntity(const String& entityId, const String& newParentId, size_t childIndex)
{
	throw Exception("Not implemented", HalleyExceptions::Entity);
}

std::pair<String, size_t> WorldSceneData::getEntityParenting(const String& entityId)
{
	return {"", 0};
}

bool WorldSceneData::isSingleRoot()
{
	return false;
}
