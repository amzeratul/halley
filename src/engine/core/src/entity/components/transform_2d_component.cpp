#include "halley/support/logger.h"

#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif

#include "halley/entity/components/transform_2d_component.h"

#include "halley/entity/world.h"
#include "halley/game/halley_statics.h"
#include "halley/navigation/world_position.h"
#include "halley/graphics/sprite/sprite.h"

using namespace Halley;

Transform2DComponent::Transform2DComponent() = default;


Transform2DComponent::Transform2DComponent(Vector2f localPosition, Angle1f localRotation, Vector2f localScale, int subWorld, float height)
	: Transform2DComponentBase(localPosition, localScale, localRotation, height, false, subWorld)
{
}

Transform2DComponent::Transform2DComponent(WorldPosition localPosition, Angle1f localRotation, Vector2f localScale, float height)
	: Transform2DComponentBase(localPosition.pos, localScale, localRotation, height, false, localPosition.subWorld)
{
}

Transform2DComponent::~Transform2DComponent()
{
	if (entity.isValid()) {
		markDirty(DirtyPropagationMode::Removed);
	}
}

void Transform2DComponent::onAddedToEntity(EntityRef& entity)
{
	this->entity = entity;
	worldPartition = entity.getWorldPartition();
	updateParentTransform();
	markDirty(DirtyPropagationMode::Added);
}

void Transform2DComponent::onHierarchyChanged()
{
	updateParentTransform();	
	markDirtyShallow(0xFF);
}

void Transform2DComponent::updateParentTransform()
{
	parentTransform = entity.hasParent() ? entity.getParent().tryGetComponent<Transform2DComponent>() : nullptr;
}

void Transform2DComponent::setLocalPosition(Vector2f v)
{
	if (position != v) {
		position = v;
		markDirty(CachedIndices::Position);
	}
}

void Transform2DComponent::setLocalScale(Vector2f v)
{
	if (scale != v) {
		scale = v;
		markDirty(CachedIndices::Scale);
	}
}

void Transform2DComponent::setLocalRotation(Angle1f v)
{
	if (rotation != v) {
		rotation = v;
		markDirty(CachedIndices::Rotation);
	}
}

void Transform2DComponent::setLocalHeight(float v)
{
	if (height != v) {
		height = v;
		markDirty(CachedIndices::Height);
	}
}

void Transform2DComponent::setFixedHeight(bool v)
{
	if (fixedHeight != v) {
		fixedHeight = v;
		setCached(CachedIndices::Height); // This is needed otherwise the system might incorrectly assume that because the flag is unset, nobody has read it
		markDirty();
	}
}

Vector2f Transform2DComponent::getGlobalPosition() const
{
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

Vector2f Transform2DComponent::getGlobalPositionWithHeight() const
{
	return getGlobalPosition() + Vector2f(0, -getGlobalHeight());
}

WorldPosition Transform2DComponent::getWorldPosition() const
{
	return WorldPosition(getGlobalPosition(), getSubWorld());
}

void Transform2DComponent::setGlobalPosition(Vector2f v)
{
	setLocalPosition(parentTransform ? parentTransform->inverseTransformPoint(v) : v);
}

void Transform2DComponent::setGlobalPosition(WorldPosition p)
{
	setGlobalPosition(p.pos);
	setSubWorld(p.subWorld);
}

Vector2f Transform2DComponent::getGlobalScale() const
{
	if (parentTransform) {
		if (!isCached(CachedIndices::Scale)) {
			setCached(CachedIndices::Scale);
			cachedGlobalScale = parentTransform->getGlobalScale() * scale;
		}
		return cachedGlobalScale;
	} else {
		return scale;
	}
}

void Transform2DComponent::setGlobalScale(Vector2f v)
{
	setLocalScale(parentTransform ? v / parentTransform->getGlobalScale() : v);
}

Angle1f Transform2DComponent::getGlobalRotation() const
{
	if (parentTransform) {
		if (!isCached(CachedIndices::Rotation)) {
			setCached(CachedIndices::Rotation);
			cachedGlobalRotation = parentTransform->getGlobalRotation() + rotation;
		}
		return cachedGlobalRotation;
	} else {
		return rotation;
	}
}

void Transform2DComponent::setGlobalRotation(Angle1f v)
{
	setLocalRotation(parentTransform ? v - parentTransform->getGlobalRotation() : v);
}

float Transform2DComponent::getGlobalHeight() const
{
	if (!fixedHeight && parentTransform) {
		if (!isCached(CachedIndices::Height)) {
			setCached(CachedIndices::Height);
			cachedGlobalHeight = parentTransform->getGlobalHeight() + height;
		}
		return cachedGlobalHeight;
	} else {
		return height;
	}
}

void Transform2DComponent::setGlobalHeight(float v)
{
	setLocalHeight(parentTransform ? v - parentTransform->getGlobalHeight() : v);
}

int Transform2DComponent::getSubWorld() const
{
	if (subWorld) {
		return subWorld.value();
	} else {
		// Default value, default to parent, or to zero if no parent
		if (parentTransform) {
			if (!isCached(CachedIndices::SubWorld)) {
				setCached(CachedIndices::SubWorld);
				cachedSubWorld = static_cast<int16_t>(parentTransform->getSubWorld());
			}
			return cachedSubWorld;
		} else {
			return 0;
		}
	}
}

void Transform2DComponent::setSubWorld(int world)
{
	if (subWorld != static_cast<int16_t>(world)) {
		subWorld = static_cast<int16_t>(world);
		markDirty(CachedIndices::SubWorld);
	}
}

Vector2f Transform2DComponent::transformPoint(const Vector2f& p) const
{
	const auto r = getGlobalRotation();
	Vector2f pos;

	if (std::abs(r.getRadians()) > 0.00001f) {
		const float anisotropy = entity.getWorld().getTransform2DAnisotropy();
		pos = getGlobalPosition() + (p * Vector2f(1.0f, 1.0f / anisotropy)).rotate(r) * Vector2f(1.0f, anisotropy) * getGlobalScale();
	} else {
		pos = getGlobalPosition() + p * getGlobalScale();
	}
	
	setCached(CachedIndices::Position); // Important: getGlobalPosition() won't cache if it's the root, but this is important for markDirty
	return pos;
}

Vector2f Transform2DComponent::inverseTransformPoint(const Vector2f& p) const
{
	const auto s = getGlobalScale();
	const auto r = getGlobalRotation();

	auto pos = (p - getGlobalPosition()) / s;

	// Degenerate cases
	if (std::abs(s.x) < 0.000001f) {
		pos.x = 0;
	}
	if (std::abs(s.y) < 0.000001f) {
		pos.y = 0;
	}

	if (std::abs(r.getRadians()) > 0.00001f) {
		const float anisotropy = entity.getWorld().getTransform2DAnisotropy();
		pos = (pos * Vector2f(1.0f, 1.0f / anisotropy)).rotate(-r) * Vector2f(1.0f, anisotropy);
	}

	setCached(CachedIndices::Position); // Important: getGlobalPosition() won't cache if it's the root, but this is important for markDirty
	return pos;
}

Rect4f Transform2DComponent::getSpriteAABB(const Sprite& sprite) const
{
	return sprite.getAABB() - sprite.getPosition() + getGlobalPosition() + Vector2f(0, -getGlobalHeight());
}

Rect4f Transform2DComponent::getSpriteUncroppedAABB(const Sprite& sprite) const
{
	return sprite.getUncroppedAABB() - sprite.getPosition() + getGlobalPosition() + Vector2f(0, -getGlobalHeight());
}

void Transform2DComponent::deserialize(const EntitySerializationContext& context, const ConfigNode& node)
{
	Transform2DComponentBase::deserialize(context, node);
	markDirty();
}

void Transform2DComponent::deserializeNetwork(const ByteSerializationContext& context, Deserializer& deserializer)
{
	Transform2DComponentBase::deserializeNetwork(context, deserializer);
	markDirty();
}

void Transform2DComponent::markDirty(CachedIndices index)
{
	markDirty(getMaskBit(index));
}

void Transform2DComponent::markDirty(uint8_t changeMask)
{
	markDirty(DirtyPropagationMode::Changed, 0, changeMask);
}

void Transform2DComponent::markDirty(DirtyPropagationMode mode, int depth, uint8_t changeMask) const
{
	// For "Changed" mode only:
	// If cachedValues is zero, it means that nobody has read this (any read MUST set cachedValues to non-zero)
	// Since nobody read it, then there's no need to do anything, or indeed to even propagate changes down
	
	if ((cachedValues & changeMask) != 0 || mode != DirtyPropagationMode::Changed) {
		markDirtyShallow(changeMask);

		// Find the child propagation mask - it might not be the same as e.g. a rotation of the parent can induce a change in position in the child
		const uint8_t childChangeMask = changeMask | 
			(hasMaskBits<CachedIndices::Position, CachedIndices::Rotation, CachedIndices::Scale>(changeMask) ? getMaskBit<CachedIndices::Position>() : 0);

		// Propagate to all children
		for (auto& c: entity.getRawChildren()) {
			if (const auto childTransform = c->tryGetComponent<Transform2DComponent>()) {
				// Propagate change on all bits of the flag, since they all potentially affect others
				childTransform->markDirty(mode, depth + 1, childChangeMask);
			}
		}

		// For the level immediately below the forced updated, the entity should also re-get parent transform as it might be new or gone
		if (depth == 1) {
			if (mode == DirtyPropagationMode::Added) {
				parentTransform = entity.getParent().tryGetComponent<Transform2DComponent>();
			} else if (mode == DirtyPropagationMode::Removed) {
				parentTransform = nullptr;
			}
		}
	}
}

void Transform2DComponent::markDirtyShallow(uint8_t changeMask) const
{
	++revision;
	if ((changeMask & (1 << static_cast<int>(CachedIndices::SubWorld))) != 0) {
		++subWorldRevision;
	}
	cachedValues = cachedValues & ~changeMask;
}

bool Transform2DComponent::isCached(CachedIndices index) const
{
	return cachedValues & (1 << int(index));
}

void Transform2DComponent::setCached(CachedIndices index) const
{
	cachedValues |= (1 << int(index));
}
