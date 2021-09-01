#pragma once

#include "halley/maths/vector2.h"
#include "halley/entity/component.h"
#include "halley/entity/entity.h"
#include "halley/file_formats/config_file.h"
#include "halley/bytes/config_node_serializer.h"
#include "components/transform2d_component_base.h"

namespace Halley
{
	class Sprite;
}

class Transform2DComponent final : public Transform2DComponentBase {
public:
	Transform2DComponent();
	explicit Transform2DComponent(Halley::Vector2f localPosition, Halley::Angle1f localRotation = {}, Halley::Vector2f localScale = Halley::Vector2f(1, 1), int subWorld = 0);
	~Transform2DComponent();

	const Halley::Vector2f& getLocalPosition() const { return position; }
	Halley::Vector2f& getLocalPosition() { return position; }
	void setLocalPosition(Halley::Vector2f v);

	const Halley::Vector2f& getLocalScale() const { return scale; }
	Halley::Vector2f& getLocalScale() { return scale; }
	void setLocalScale(Halley::Vector2f v);

	const Halley::Angle1f& getLocalRotation() const { return rotation; }
	Halley::Angle1f& getLocalRotation() { return rotation; }
	void setLocalRotation(Halley::Angle1f v);

	Halley::Vector2f getGlobalPosition() const;
	void setGlobalPosition(Halley::Vector2f v);

	Halley::Vector2f getGlobalScale() const;
	void setGlobalScale(Halley::Vector2f v);

	Halley::Angle1f getGlobalRotation() const;
	void setGlobalRotation(Halley::Angle1f v);

	int getSubWorld() const;
	void setSubWorld(int subWorld);

	Halley::Vector2f transformPoint(const Halley::Vector2f& p) const;
	Halley::Vector2f inverseTransformPoint(const Halley::Vector2f& p) const;

	Halley::Rect4f getSpriteAABB(const Halley::Sprite& sprite) const;
	Halley::Rect4f getSpriteUncroppedAABB(const Halley::Sprite& sprite) const;

	void onAddedToEntity(Halley::EntityRef& entity);
	void onHierarchyChanged();

	uint16_t getRevision() const { return revision; }
	uint8_t getWorldPartition() const { return worldPartition; }

	void deserialize(const Halley::ConfigNodeSerializationContext& context, const Halley::ConfigNode& node);

private:
	friend class Halley::EntityRef;

	mutable Transform2DComponent* parentTransform = nullptr;
	mutable uint16_t revision = 0;
	mutable uint8_t worldPartition = 0;

	mutable uint8_t cachedValues = 0;
	mutable int cachedSubWorld = 0;
	mutable Halley::Vector2f cachedGlobalPos;
	mutable Halley::Vector2f cachedGlobalScale;
	mutable Halley::Angle1f cachedGlobalRotation;

	mutable Halley::EntityRef entity;

	enum class CachedIndices {
		Position,
		Scale,
		Rotation,
		SubWorld
	};

	enum class DirtyPropagationMode {
		Changed,
		Added,
		Removed
	};

	void updateParentTransform();
	void markDirty(DirtyPropagationMode mode = DirtyPropagationMode::Changed, int depth = 0) const;
	void markDirtyShallow() const;
	bool isCached(CachedIndices index) const;
	void setCached(CachedIndices index) const;
};
