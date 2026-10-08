#include "halley/entity/family.h"
#include "halley/entity/family_binding.h"

using namespace Halley;

Family::Family(FamilyMaskType inclusionMask, std::optional<FamilyMaskType> exclusionMask, FamilyMaskType optionalMask)
	: inclusionMask(inclusionMask)
	, exclusionMask(exclusionMask)
	, optionalMask(optionalMask)
{
}

void Family::addOnEntitiesAdded(FamilyBindingBase* bind)
{
	addEntityCallbacks.push_back(bind);
	bind->onEntitiesAdded(elems, elemCount);
}

void Family::removeOnEntityAdded(FamilyBindingBase* bind)
{
	addEntityCallbacks.erase(std::remove(addEntityCallbacks.begin(), addEntityCallbacks.end(), bind), addEntityCallbacks.end());
}

void Family::addOnEntitiesRemoved(FamilyBindingBase* bind)
{
	removeEntityCallbacks.push_back(bind);
}

void Family::removeOnEntityRemoved(FamilyBindingBase* bind)
{
	removeEntityCallbacks.erase(std::remove(removeEntityCallbacks.begin(), removeEntityCallbacks.end(), bind), removeEntityCallbacks.end());
}

void Family::addOnEntitiesReloaded(FamilyBindingBase* bind)
{
	modifiedEntityCallbacks.push_back(bind);
}

void Family::removeOnEntitiesReloaded(FamilyBindingBase* bind)
{
	modifiedEntityCallbacks.erase(std::remove(modifiedEntityCallbacks.begin(), modifiedEntityCallbacks.end(), bind), modifiedEntityCallbacks.end());
}

void Family::notifyAdd(void* entities, size_t count)
{
	for (auto& c: addEntityCallbacks) {
		c->onEntitiesAdded(entities, count);
	}
}

void Family::notifyRemove(void* entities, size_t count)
{
	for (auto& c: removeEntityCallbacks) {
		c->onEntitiesRemoved(entities, count);
	}
}

void Family::notifyReload(void* entities, size_t count)
{
	for (auto& c : modifiedEntityCallbacks) {
		c->onEntitiesReloaded(entities, count);
	}
}

OptionalLite<size_t> Family::findElementInIndex(EntityId id) const
{
	const auto iter = index.find(id.getIndex());
	if (iter != index.end()) {
		return static_cast<size_t>(iter->second);
	}

	return std::nullopt;
}

Vector<uint8_t>& Family::getScratchBitSet()
{
	static thread_local Vector<uint8_t> scratch;
	return scratch;
}

void Family::setIndexed()
{
	if (!indexed) {
		indexed = true;
		rebuildIndex();
	}
}

void Family::removeEntity(EntityId::Index idx)
{
	toRemove.push_back(idx);
}

void Family::reloadEntity(EntityId::Index idx)
{
	toReload.push_back(idx);
}
