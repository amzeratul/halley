#define DONT_INCLUDE_HALLEY_HPP
#include "entity/components/transform_2d_component.h"

using namespace Halley;

Transform2DComponent::Transform2DComponent() = default;

Transform2DComponent::Transform2DComponent(Vector2f localPosition, Angle1f localRotation, Vector2f localScale, int subWorld)
	: Transform2DComponentBase(localPosition, localRotation, localScale, subWorld)
{
}

Transform2DComponent::Transform2DComponent(Transform2DComponent& parentTransform, Vector2f localPosition, Angle1f localRotation, Vector2f localScale, int subWorld)
	: Transform2DComponentBase(localPosition, localRotation, localScale, subWorld)
{
	setParent(parentTransform, true);
}

Transform2DComponent::Transform2DComponent(EntityId parentId, World& world, Vector2f localPosition, Angle1f localRotation, Vector2f localScale, int subWorld)
	: Transform2DComponentBase(localPosition, localRotation, localScale, subWorld)
{
	setParent(parentId, world, true);
}

Vector2f Transform2DComponent::getGlobalPosition() const
{
	if (parentTransform) {
		return transformPoint(position);
	} else {
		return position;
	}
}

void Transform2DComponent::setGlobalPosition(Vector2f v)
{
	if (parentTransform) {
		position = inverseTransformPoint(v);
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

int Transform2DComponent::getSubWorld() const
{
	return subWorld;
}

void Transform2DComponent::setSubWorld(int world)
{
	subWorld = world;
}

Vector2f Transform2DComponent::transformPoint(const Vector2f& p) const
{
	// TODO, do this properly
	return parentTransform->getGlobalPosition() + p;
}

Vector2f Transform2DComponent::inverseTransformPoint(const Vector2f& p) const
{
	// TODO, do this properly
	return p - parentTransform->getGlobalPosition();
}

void Transform2DComponent::setParent(EntityId newParentId, World& world, bool keepLocalPosition)
{
	if (parentId != newParentId) {
		setParent(world.getEntity(parentId).getComponent<Transform2DComponent>(), keepLocalPosition);
	}
}

void Transform2DComponent::setParent(Transform2DComponent& newParentTransform, bool keepLocalPosition)
{
	if (parentTransform != &newParentTransform) {
		// Unparent from old
		setParent(keepLocalPosition);

		// Set id
		parentId = newParentTransform.myId;

		// Reparent
		parentTransform = &newParentTransform;
		parentTransform->childIds.push_back(myId);

		if (!keepLocalPosition) {
			setGlobalPosition(getLocalPosition());
			setGlobalRotation(getLocalRotation());
			setGlobalScale(getLocalScale());
		}
	}
}

void Transform2DComponent::setParent(bool keepLocalPosition)
{
	if (parentTransform) {
		if (!keepLocalPosition) {
			setLocalPosition(getGlobalPosition());
			setLocalRotation(getGlobalRotation());
			setLocalScale(getGlobalScale());
		}
		
		auto& siblings = parentTransform->childIds;
		siblings.erase(std::remove(siblings.begin(), siblings.end(), myId), siblings.end());
		parentTransform = nullptr;
	}
	parentId = EntityId();
}

void Transform2DComponent::addChild(EntityId parentId, World& world, bool keepLocalPosition)
{
	world.getEntity(parentId).getComponent<Transform2DComponent>().setParent(*this, keepLocalPosition);
}

void Transform2DComponent::addChild(Transform2DComponent& childTransform, bool keepLocalPosition)
{
	childTransform.setParent(*this, keepLocalPosition);
}

void Transform2DComponent::detachChildren(World& world)
{
	auto childIdsCopy = childIds;
	for (auto& childId: childIdsCopy) {
		world.getEntity(childId).getComponent<Transform2DComponent>().setParent();
	}
}

void Transform2DComponent::destroyTree(World& world, bool includingMe)
{
	auto childIdsCopy = childIds;
	for (auto& childId: childIdsCopy) {
		world.getEntity(childId).getComponent<Transform2DComponent>().destroyTree(world);
	}
	setParent(true);
	if (includingMe) {
		world.destroyEntity(myId);
	}
}

void Transform2DComponent::onAddedToEntity(EntityRef& entity)
{
	myId = entity.getEntityId();
}
