#include <halley/data_structures/memory_pool.h>
#include "entity.h"


#include "world.h"
#include "components/transform_2d_component.h"
#include "components/network_component.h"


using namespace Halley;

Entity::Entity()
	: dirty(false)
	, alive(true)
	, serializable(true)
	, reloaded(false)
	, active(true)
	, selectable(true)
{

}

Entity::~Entity() = default;

void Entity::destroyComponents(ComponentDeleterTable& table)
{
	for (auto& component : components) {
		deleteComponent(component.second, component.first, table);
	}
	components.clear();
	liveComponents = 0;
}

void Entity::removeComponentById(World& world, int id)
{
	for (uint8_t i = 0; i < liveComponents; ++i) {
		if (components[i].first == id) {
			removeComponentAt(i);
			markDirty(world);
			return;
		}
	}
}

void Entity::addComponent(Component* component, int id)
{
	// Note that we can't simply delete component here in case of an exception, as deleting it requires using the deleter table from the world
	
	if (liveComponents == std::numeric_limits<decltype(liveComponents)>::max()) {
		throw Exception("Too many components in Entity. Memory leak has occurred.", HalleyExceptions::Entity);
	}

	// Ensure it's not already there
	for (uint8_t i = 0; i < liveComponents; ++i) {
		if (components[i].first == id) {
			throw Exception("Component already added to Entity. Memory leak has occurred.", HalleyExceptions::Entity);
		}
	}
	
	// Put it at the back of the list...
	components.push_back(std::pair<int, Component*>(id, component));

	// ...if there's dead components, swap with the first dead component...
	if (static_cast<size_t>(liveComponents) < components.size()) {
		std::swap(components[liveComponents], components.back());
	}

	// ...and increase the list, therefore putting it in living component territory
	++liveComponents;
}

void Entity::removeComponentAt(int i)
{
	// Put it at the end of the list of living components... (guaranteed to swap with living component)
	std::swap(components[i], components[static_cast<size_t>(liveComponents) - 1]);

	// ...then shrink that list, therefore moving it into dead component territory
	--liveComponents;
}

void Entity::removeAllComponents(World& world)
{
	liveComponents = 0;
	markDirty(world);
}

void Entity::deleteComponent(Component* component, int id, ComponentDeleterTable& table)
{
	TypeDeleterBase* deleter = table.get(id);
	deleter->callDestructor(component);
	PoolPool::getPool(deleter->getSize())->free(component);
}

void Entity::keepOnlyComponentsWithIds(const std::vector<int>& ids, World& world)
{
	for (uint8_t i = 0; i < liveComponents; ++i) {
		if (std::find(ids.begin(), ids.end(), components[i].first) == ids.end()) {
			std::swap(components[i], components[liveComponents - 1]);
			--liveComponents;
			--i;
		}
	}
	
	markDirty(world);
}

void Entity::onReady()
{
}

void Entity::markDirty(World& world)
{
	if (!dirty) {
		dirty = true;
		world.onEntityDirty();
	}
}

ComponentDeleterTable& Entity::getComponentDeleterTable(World& world)
{
	return world.getComponentDeleterTable();
}

void Entity::setParent(Entity* newParent, bool propagate, size_t childIdx)
{
	Expects(newParent != this);
	Expects(isAlive());
	
	if (parent != newParent) {
		// Unparent from old
		if (parent) {
			Expects(parent->isAlive());
			
			auto& siblings = parent->children;
			siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
			parent->propagateChildrenChange();
			parent = nullptr;
		}

		// Reparent
		if (newParent) {
			parent = newParent;
			if (worldPartition != newParent->worldPartition) {
				propagateChildWorldPartition(newParent->worldPartition);
			}
			if (childIdx >= parent->children.size()) {
				parent->children.push_back(this);
			} else {
				parent->children.insert(parent->children.begin() + childIdx, this);
			}
			parent->propagateChildrenChange();
		}

		if (propagate) {
			markHierarchyDirty();
		}
	}
}

void Entity::addChild(Entity& child)
{
	child.setParent(this);
}

void Entity::detachChildren()
{
	auto childrenCopy = std::move(children);
	for (auto& child : childrenCopy) {
		child->setParent(nullptr);
	}
	children.clear();
}

void Entity::markHierarchyDirty()
{
	hierarchyRevision++;

	// Notify transform
	auto transform = tryGetComponent<Transform2DComponent>();
	if (transform) {
		transform->onHierarchyChanged();
	}
	
	for (auto& child: children) {
		child->markHierarchyDirty();
	}
}

void Entity::propagateChildrenChange()
{
	// Could be recursive, but want to make sure I'm not paying for function calls here
	for (Entity* cur = this; cur; cur = cur->parent) {
		cur->childrenRevision++;
	}
}

void Entity::propagateChildWorldPartition(uint8_t newWorldPartition)
{
	worldPartition = newWorldPartition;
	for (auto& child : children) {
		child->propagateChildWorldPartition(newWorldPartition);
	}
}

FamilyMaskType Entity::getMask() const
{
	return mask;
}

void Entity::refresh(MaskStorage& storage, ComponentDeleterTable& table)
{
	if (dirty) {
		dirty = false;

		// Delete stale components
		for (size_t i = liveComponents; i < components.size(); ++i) {
			deleteComponent(components[i].second, components[i].first, table);
		}
		components.resize(liveComponents);

		// Re-generate mask
		auto m = FamilyMask::RealType();
		for (auto i : components) {
			FamilyMask::setBit(m, i.first);
		}
		mask = FamilyMaskType(m, storage);

		// Notify parent
		if (parent) {
			parent->propagateChildrenChange();
		}
	}
}

EntityId Entity::getEntityId() const
{
	if (!entityId.isValid()) {
		//throw Exception("Entity ID not yet assigned - are you using this before it's spawned?");
	}
	return entityId;
}

void Entity::destroy()
{
	doDestroy(true);
}

void Entity::sortChildrenByInstanceUUIDs(const std::vector<UUID>& uuids)
{
	const size_t nChildren = children.size();

	// Check if there's any work to be done
	if (nChildren == uuids.size()) {
		bool allMatch = true;
		for (size_t i = 0; i < nChildren; ++i) {
			if (children[i]->instanceUUID != uuids[i]) {
				allMatch = false;
				break;
			}
		}
		if (allMatch) {
			// Nothing to do here, avoid memory allocation and sorting!
			return;
		}
	}

	Vector<std::pair<Entity*, size_t>> pairs(nChildren);
	
	for (size_t i = 0; i < nChildren; ++i) {
		const size_t idx = std::find(uuids.begin(), uuids.end(), children[i]->instanceUUID) - uuids.begin();
		pairs[i] = std::make_pair(children[i], idx);
	}

	std::sort(pairs.begin(), pairs.end(), [](const std::pair<Entity*, size_t>& a, const std::pair<Entity*, size_t>& b)
	{
		if (a.second != b.second) {
			return a.second < b.second;
		}
		return a.first < b.first;
	});

	for (size_t i = 0; i < nChildren; ++i) {
		children[i] = pairs[i].first;
	}
}

bool Entity::isEmpty() const
{
	return liveComponents == 0 && children.empty();
}

void Entity::setupNetwork(EntityRef& ref, uint8_t peerId)
{
	auto* networkComponent = tryGetComponent<NetworkComponent>();
	if (networkComponent) {
		networkComponent->ownerId = peerId;
	} else {
		NetworkComponent component;
		component.ownerId = peerId;
		ref.addComponent(std::move(component));
	}
}

std::optional<uint8_t> Entity::getOwnerPeerId() const
{
	auto* networkComponent = tryGetComponent<NetworkComponent>();
	if (networkComponent) {
		networkComponent->ownerId;
	}
	return {};
}

void Entity::doDestroy(bool updateParenting)
{
	Expects(alive);
	
	if (updateParenting) {
		setParent(nullptr, false);
	}

	for (auto& c: children) {
		c->doDestroy(false);
	}
	children.clear();
	
	alive = false;
	dirty = true;
}

bool Entity::hasBit(const World& world, int index) const
{
	return FamilyMask::hasBit(mask, index, world.getMaskStorage());
}

void EntityRef::setReloaded()
{
	Expects(entity);
	entity->reloaded = true;

	world->setEntityReloaded();
}
