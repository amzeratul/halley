#define DONT_INCLUDE_HALLEY_HPP
#include "entity/components/transform_2d_component.h"

using namespace Halley;

Transform2DComponent::Transform2DComponent()
{
}

Vector2f Transform2DComponent::getGlobalPosition() const
{
	if (parentTransform) {
		// TODO, do this properly
		return position + parentTransform->getGlobalPosition();
	} else {
		return position;
	}
}

void Transform2DComponent::setGlobalPosition(Vector2f v)
{
	if (parentTransform) {
		position = v - parentTransform->getGlobalPosition();
	} else {
		position = v;
	}
}

Vector2f Transform2DComponent::getGlobalScale() const
{
	// TODO
	return scale;
}

void Transform2DComponent::setGlobalScale(Vector2f v)
{
	// TODO
	scale = v;
}

Angle1f Transform2DComponent::getGlobalRotation() const
{
	// TODO
	return rotation;
}

void Transform2DComponent::setGlobalRotation(Angle1f v)
{
	// TODO
	rotation = v;
}

void Transform2DComponent::setParent(EntityId newParentId, World& world)
{
	if (parentId != newParentId) {
		setParent(world.getEntity(parentId).getComponent<Transform2DComponent>());
	}
}

void Transform2DComponent::setParent(Transform2DComponent& newParentTransform)
{
	if (parentTransform != &newParentTransform) {
		// Unparent from old
		setParent();

		// Set id
		parentId = newParentTransform.myId;

		// Reparent
		parentTransform = &newParentTransform;
		parentTransform->childIds.push_back(myId);
	}
}

void Transform2DComponent::setParent()
{
	if (parentTransform) {
		auto& siblings = parentTransform->childIds;
		siblings.erase(std::remove(siblings.begin(), siblings.end(), myId), siblings.end());
		parentTransform = nullptr;
	}
	parentId = EntityId();
}

void Transform2DComponent::addChild(EntityId parentId, World& world)
{
	world.getEntity(parentId).getComponent<Transform2DComponent>().setParent(*this);
}

void Transform2DComponent::addChild(Transform2DComponent& childTransform)
{
	childTransform.setParent(*this);
}

void Transform2DComponent::detachChildren(World& world)
{
	auto childIdsCopy = childIds;
	for (auto& childId: childIdsCopy) {
		world.getEntity(childId).getComponent<Transform2DComponent>().setParent();
	}
}

void Transform2DComponent::destroyTree(World& world)
{
	auto childIdsCopy = childIds;
	for (auto& childId: childIdsCopy) {
		world.getEntity(childId).getComponent<Transform2DComponent>().destroyTree(world);
	}
	setParent();
	world.destroyEntity(myId);
}

void Transform2DComponent::onAddedToEntity(Halley::EntityRef& entity)
{
	myId = entity.getEntityId();
}
