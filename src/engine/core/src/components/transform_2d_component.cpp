#include "halley/maths/vector2.h"
#include "halley/entity/component.h"
#include "halley/entity/entity.h"
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
