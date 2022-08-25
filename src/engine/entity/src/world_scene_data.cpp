#include "world_scene_data.h"
using namespace Halley;

WorldSceneData::WorldSceneData(World& world)
	: world(world)
{
}

std::optional<ISceneData::EntityNodeData> WorldSceneData::tryGetWriteableEntityNodeData(const String& id)
{
	throw Exception("Not implemented", HalleyExceptions::Entity);
}

std::pair<Vector<UUID>, Vector<EntityData*>> WorldSceneData::getWriteableEntityDatas(gsl::span<const UUID> ids)
{
	throw Exception("Not implemented", HalleyExceptions::Entity);
}

std::optional<ISceneData::ConstEntityNodeData> WorldSceneData::tryGetEntityNodeData(const String& id)
{
	throw Exception("Not implemented", HalleyExceptions::Entity);
}

void WorldSceneData::reloadEntities(gsl::span<const String> ids, gsl::span<const EntityData*> datas)
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
