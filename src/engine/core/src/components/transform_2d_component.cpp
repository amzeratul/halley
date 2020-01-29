#include "halley/maths/vector2.h"
#include "halley/entity/component.h"
#include "halley/entity/entity.h"
#include "halley/entity/world.h"
#include "halley/core/resources/resources.h"
#include "halley/file_formats/config_file.h"
#include "halley/bytes/config_node_serializer.h"

#define DONT_INCLUDE_HALLEY_HPP
#include "components/transform_2d_component.h"

using namespace Halley;

Transform2DComponent::Transform2DComponent()
{
}

Vector2f Transform2DComponent::getGlobalPosition() const
{
	// TODO
	return position;
}

void Transform2DComponent::setGlobalPosition(Vector2f v)
{
	// TODO
	position = v;
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

void Transform2DComponent::init(EntityId id)
{
	myId = id;
}

void Transform2DComponent::setParent(EntityId newParentId, World& world)
{
	if (parentId != newParentId) {
	
		// Unparent from old
		setParent();

		// Set id
		parentId = newParentId;

		// Reparent
		if (parentId.isValid()) {
			parentTransform = &world.getEntity(parentId.value()).getComponent<Transform2DComponent>();
			parentTransform->childIds.push_back(myId);
		}
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
	// TODO
}

void Transform2DComponent::addChild(Transform2DComponent& parentTransform)
{
	// TODO
}

void Transform2DComponent::detachChildren()
{
	// TODO
}
