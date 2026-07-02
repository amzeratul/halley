#pragma once

#include "halley/maths/vector2.h"
#include "halley/entity/component.h"
#include "halley/entity/entity.h"
#include "halley/file_formats/config_file.h"
#include "halley/bytes/config_node_serializer.h"
#include "components/transform2d_component_base.h"

namespace Halley
{
	class WorldPosition;
	class Sprite;
}

class Transform2DComponent final : public Transform2DComponentBase<Transform2DComponent> {
public:
	Transform2DComponent();
	explicit Transform2DComponent(Halley::Vector2f localPosition, Halley::Angle1f localRotation = {}, Halley::Vector2f localScale = Halley::Vector2f(1, 1), int subWorld = 0, float height = 0);
	explicit Transform2DComponent(Halley::WorldPosition localPosition, Halley::Angle1f localRotation = {}, Halley::Vector2f localScale = Halley::Vector2f(1, 1), float height = 0);
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

	float getLocalHeight() const { return height; }
	float& getLocalHeight() { return height; }
	void setLocalHeight(float v);

	bool isFixedHeight() const { return fixedHeight; }
	void setFixedHeight(bool fixed);

	Halley::Vector2f getGlobalPosition() const;
	Halley::Vector2f getGlobalPositionWithHeight() const;
	Halley::WorldPosition getWorldPosition() const;
	void setGlobalPosition(Halley::Vector2f v);
	void setGlobalPosition(Halley::WorldPosition p);

	Halley::Vector2f getGlobalScale() const;
	void setGlobalScale(Halley::Vector2f v);

	Halley::Angle1f getGlobalRotation() const;
	void setGlobalRotation(Halley::Angle1f v);
	
	float getGlobalHeight() const;
	void setGlobalHeight(float v);

	int getSubWorld() const;
	void setSubWorld(int subWorld);

	bool isTranslationOnly() const;

	Halley::Vector2f transformPoint(const Halley::Vector2f& p) const;
	Halley::Vector2f transformPointNoRotate(const Halley::Vector2f& p) const;
	Halley::Vector2f transformPointWithHeight(const Halley::Vector2f& p) const;
	Halley::Vector2f transformPointWithHeightNoRotate(const Halley::Vector2f& p) const;
	Halley::Vector2f inverseTransformPoint(const Halley::Vector2f& p) const;

	Halley::Rect4f getSpriteAABB(const Halley::Sprite& sprite) const;
	Halley::Rect4f getSpriteUncroppedAABB(const Halley::Sprite& sprite) const;

	void onAddedToEntity(Halley::EntityRef& entity);
	void onHierarchyChanged();
	void onWorldPartitionChanged();

	uint16_t getRevision() const { return revision; }
	uint16_t getSubWorldRevision() const { return subWorldRevision; }
	Halley::WorldPartitionId getWorldPartition() const { return worldPartition; }

	uint16_t getDepth() const { return depth; }

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node);
	void deserializeNetwork(const Halley::ByteSerializationContext& context, Halley::Deserializer& deserializer);

	void markDirty(uint8_t changeMask = (uint8_t)0xFF);

private:
	friend class Halley::EntityRef;

	mutable Transform2DComponent* parentTransform = nullptr;
	mutable Halley::WorldPartitionId worldPartition = 0;

	mutable int16_t cachedSubWorld = 0;
	mutable float cachedGlobalHeight;
	mutable Halley::Vector2f cachedGlobalPos;
	mutable Halley::Vector2f cachedGlobalScale;
	mutable Halley::Angle1f cachedGlobalRotation;
	mutable uint16_t depth = 0;

	mutable Halley::EntityRef entity;

	enum class CachedIndices: uint8_t {
		Position,
		Scale,
		Rotation,
		SubWorld,
		Height
	};

	enum class DirtyPropagationMode: uint8_t {
		Changed,
		Added,
		Removed
	};

	void updateParentTransform();
	void markDirty(CachedIndices index);
	void markDirty(DirtyPropagationMode mode, int dirtyDepth = 0, uint8_t changeMask = (uint8_t)0xFF) const;
	void markDirtyShallow(uint8_t changeMask) const;
	bool isCached(CachedIndices index) const;
	void setCached(CachedIndices index) const;

	template<CachedIndices c, CachedIndices ... cs>
	inline static constexpr uint8_t getMaskBits()
	{
		uint8_t result = getMaskBit<c>();
		if constexpr (sizeof...(cs) > 0) {
			result |= getMaskBits<cs...>();
		}
		return result;
	}

	template<CachedIndices c>
	inline static constexpr uint8_t getMaskBit()
	{
		return static_cast<uint8_t>(1 << static_cast<uint8_t>(c));
	}

	inline static constexpr uint8_t getMaskBit(CachedIndices index)
	{
		return static_cast<uint8_t>(1 << static_cast<uint8_t>(index));
	}

	inline static constexpr bool hasMaskBit(uint8_t mask, CachedIndices index)
	{
		return (getMaskBit(index) & mask) != 0;
	}

	template<CachedIndices ... cs>
	inline static constexpr bool hasMaskBits(uint8_t mask)
	{
		return (getMaskBits<cs...>() & mask) != 0;
	}
};
