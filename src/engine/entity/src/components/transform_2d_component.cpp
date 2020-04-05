#define DONT_INCLUDE_HALLEY_HPP
#include "halley/support/logger.h"
#include "components/transform_2d_component.h"
#include "halley/core/graphics/sprite/sprite.h"

using namespace Halley;

Transform2DComponent::Transform2DComponent() = default;

Transform2DComponent::Transform2DComponent(Vector2f localPosition, Angle1f localRotation, Vector2f localScale, int subWorld)
	: Transform2DComponentBase(localPosition, localRotation, localScale, subWorld)
{
}

Transform2DComponent::~Transform2DComponent()
{
}

void Transform2DComponent::setLocalPosition(Halley::Vector2f v)
{
	position = v;
	markDirty();
}

void Transform2DComponent::setLocalScale(Halley::Vector2f v)
{
	scale = v;
	markDirty();
}

void Transform2DComponent::setLocalRotation(Halley::Angle1f v)
{
	rotation = v;
	markDirty();
}

Vector2f Transform2DComponent::getGlobalPosition() const
{
	checkDirty();
	if (parentTransform) {
		if (!isCached(CachedIndices::Position)) {
			setCached(CachedIndices::Position);
			cachedGlobalPos = parentTransform->transformPoint(position);
		}
		return cachedGlobalPos;
	} else {
		return position;
	}
}

void Transform2DComponent::setGlobalPosition(Vector2f v)
{
	checkDirty();
	if (parentTransform) {
		position = parentTransform->inverseTransformPoint(v);
	} else {
		position = v;
	}
	markDirty();
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
	markDirty();
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
	markDirty();
}

int Transform2DComponent::getSubWorld() const
{
	if (subWorld != INT_MIN) {
		return subWorld;
	} else {
		// Default value, default to parent, or to zero if no parent
		checkDirty();
		if (parentTransform) {
			if (!isCached(CachedIndices::SubWorld)) {
				setCached(CachedIndices::SubWorld);
				cachedSubWorld = parentTransform->getSubWorld();
			}
			return cachedSubWorld;
		} else {
			return 0;
		}
	}
}

void Transform2DComponent::setSubWorld(int world)
{
	subWorld = world;
	markDirty();
}

Vector2f Transform2DComponent::transformPoint(const Vector2f& p) const
{
	// TODO, do this properly
	return getGlobalPosition() + p;
}

Vector2f Transform2DComponent::inverseTransformPoint(const Vector2f& p) const
{
	// TODO, do this properly
	return p - getGlobalPosition();
}

Rect4f Transform2DComponent::getSpriteAABB(const Sprite& sprite) const
{
	return sprite.getAABB() - sprite.getPosition() + getGlobalPosition();
}

void Transform2DComponent::onAddedToEntity(EntityRef& entity)
{
	this->entity = entity;
}

void Transform2DComponent::deserialize(ConfigNodeSerializationContext& context, const ConfigNode& node)
{
	Transform2DComponentBase::deserialize(context, node);
	markDirty();
}

void Transform2DComponent::checkDirty() const
{
	const auto curRev = entity.getHierarchyRevision();
	if (hierarchyRevision != curRev) {
		hierarchyRevision = curRev;
		parentTransform = entity.hasParent() ? entity.getParent().tryGetComponent<Transform2DComponent>() : nullptr;
		markDirty();
	}
}

void Transform2DComponent::markDirty() const
{
	++revision;
	cachedValues = 0;
}

bool Transform2DComponent::isCached(CachedIndices index) const
{
	return cachedValues & (1 << int(index));
}

void Transform2DComponent::setCached(CachedIndices index) const
{
	cachedValues |= (1 << int(index));
}
