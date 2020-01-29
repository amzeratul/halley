#pragma once

#include "components/transform2d_component_base.h"

class Transform2DComponent : public Transform2DComponentBase {
public:
	Transform2DComponent();

	const Halley::Vector2f& getLocalPosition() const { return position; }
	Halley::Vector2f& getLocalPosition() { return position; }
	void setLocalPosition(Halley::Vector2f v) { position = v; }

	const Halley::Vector2f& getLocalScale() const { return scale; }
	Halley::Vector2f& getLocalScale() { return scale; }
	void setLocalScale(Halley::Vector2f v) { scale = v; }

	const Halley::Angle1f& getLocalRotation() const { return rotation; }
	Halley::Angle1f& getLocalRotation() { return rotation; }
	void setLocalRotation(Halley::Angle1f v) { rotation = v; }

	Halley::Vector2f getGlobalPosition() const;
	void setGlobalPosition(Halley::Vector2f v);

	Halley::Vector2f getGlobalScale() const;
	void setGlobalScale(Halley::Vector2f v);

	Halley::Angle1f getGlobalRotation() const;
	void setGlobalRotation(Halley::Angle1f v);

	void init(Halley::EntityId myId);

	Halley::Maybe<Halley::EntityId> getParent() const { return parentId; }
	void setParent(Halley::EntityId parentId, Halley::World& world);
	void setParent(Transform2DComponent& parentTransform);
	void setParent();
	
	std::vector<Halley::EntityId> getChildren() const { return childIds; }
	void addChild(Halley::EntityId parentId, Halley::World& world);
	void addChild(Transform2DComponent& parentTransform);
	void detachChildren();

private:
	friend class Entity;

	Transform2DComponent* parentTransform = nullptr;
	Halley::EntityId myId;
	Halley::EntityId parentId;
	std::vector<Halley::EntityId> childIds;
};
