#pragma once

#include "halley/maths/vector2.h"
#include "halley/entity/component.h"
#include "halley/entity/entity.h"
#include "halley/entity/world.h"
#include "halley/core/resources/resources.h"
#include "halley/file_formats/config_file.h"
#include "halley/bytes/config_node_serializer.h"
#include "components/transform2d_component_base.h"

class Transform2DComponent : public Transform2DComponentBase {
public:
	Transform2DComponent();
	explicit Transform2DComponent(Halley::Vector2f localPosition, Halley::Angle1f localRotation = {}, Halley::Vector2f localScale = Halley::Vector2f(1, 1));
	Transform2DComponent(Transform2DComponent& parentTransform, Halley::Vector2f localPosition, Halley::Angle1f localRotation = {}, Halley::Vector2f localScale = Halley::Vector2f(1, 1));
	Transform2DComponent(Halley::EntityId parentId, Halley::World& world, Halley::Vector2f localPosition, Halley::Angle1f localRotation = {}, Halley::Vector2f localScale = Halley::Vector2f(1, 1));

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

	Halley::Maybe<Halley::EntityId> getParent() const { return parentId; }
	void setParent(Halley::EntityId parentId, Halley::World& world, bool keepLocalPosition = false);
	void setParent(Transform2DComponent& parentTransform, bool keepLocalPosition = false);
	void setParent(bool keepLocalPosition = false);
	
	std::vector<Halley::EntityId> getChildren() const { return childIds; }
	void addChild(Halley::EntityId parentId, Halley::World& world);
	void addChild(Transform2DComponent& childTransform);
	void detachChildren(Halley::World& world);

	void destroyTree(Halley::World& world);

private:
	friend class Halley::EntityRef;

	Transform2DComponent* parentTransform = nullptr;
	Halley::EntityId myId;
	Halley::EntityId parentId;
	std::vector<Halley::EntityId> childIds;

	void onAddedToEntity(Halley::EntityRef& entity);
};
