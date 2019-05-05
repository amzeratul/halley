/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include "halley/core/graphics/camera.h"
#include "graphics/render_target/render_target.h"

using namespace Halley;


Camera::Camera()
{
}


Camera::Camera(Vector2f pos, Angle1f angle)
	: pos(pos)
{
	setRotation(angle);
}

Camera::Camera(Vector3f pos, Quaternion quat)
	: pos(pos)
	, rotation(quat)
{
}


Camera& Camera::setPosition(Vector2f pos)
{
	this->pos = Vector3f(pos);
	return *this;
}

Camera& Camera::setPosition(Vector3f pos)
{
	this->pos = pos;
	return *this;
}


Camera& Camera::setRotation(Angle1f angle)
{
	rotation = Quaternion(Vector3f(0, 0, 1), angle);
	return *this;
}

Camera& Camera::setRotation(Quaternion quat)
{
	rotation = quat;
	return *this;
}


Camera& Camera::setZoom(float zoom)
{
	this->zoom = zoom;
	return *this;
}

Camera& Camera::resetRenderTarget()
{
	renderTarget = nullptr;
	return *this;
}

Camera& Camera::setRenderTarget(RenderTarget& target)
{
	renderTarget = &target;
	return *this;
}

Camera& Camera::resetViewPort()
{
	viewPort.reset();
	return *this;
}

Camera& Camera::setViewPort(Rect4i v)
{
	viewPort = v;
	return *this;
}

Angle1f Camera::getZAngle() const
{
	const auto v = rotation * Vector3f(1, 0, 0);
	const auto v2d = Vector2f(v.x, v.y);
	return v2d.angle();
}

void Camera::updateProjection(bool flipVertical)
{
	Vector2i area = getActiveViewPort().getSize();

	// Setup projection
	const float w = float(area.x);
	const float h = float(area.y);
	projection = Matrix4f::makeOrtho2D(-w/2, w/2, flipVertical ? h/2 : -h/2, flipVertical ? -h/2 : h/2, -1000, 1000);

	// Camera properties
	if (zoom != 1.0f) {
		projection.scale(Vector3f(zoom, zoom, zoom));
	}
	if (rotation != Quaternion()) {
		projection.rotate(rotation.conjugated());
	}
	if (pos != Vector3f()) {
		projection.translate(-pos);
	}
}

RenderTarget& Camera::getActiveRenderTarget() const
{
	Expects(rendering);
	return renderTarget ? *renderTarget : *defaultRenderTarget;
}

RenderTarget* Camera::getRenderTarget() const
{
	return renderTarget;
}

Rect4i Camera::getActiveViewPort() const
{
	auto targetViewPort = getActiveRenderTarget().getViewPort();
	if (viewPort) {
		return viewPort.get().intersection(targetViewPort);
	} else {
		return targetViewPort;
	}
}

Rect4f Camera::getClippingRectangle() const
{
	const auto angle = getZAngle();
	auto vp = getActiveViewPort();
	auto halfSize = Vector2f(vp.getSize()) / (zoom * 2);
	auto a = halfSize.rotate(angle);
	auto b = Vector2f(-halfSize.x, halfSize.y).rotate(angle);
	auto rotatedHalfSize = Vector2f(std::max(std::abs(a.x), std::abs(b.x)), std::max(std::abs(a.y), std::abs(b.y)));
	Vector2f pos2d(pos.x, pos.y);
	return Rect4f(pos2d - rotatedHalfSize, pos2d + rotatedHalfSize);
}

Vector2f Camera::screenToWorld(Vector2f p, Rect4f viewport) const
{
	const auto angle = getZAngle();
	Vector2f pos2d(pos.x, pos.y);
	return ((p - viewport.getCenter()) / zoom).rotate(angle) + pos2d;
}

Vector2f Camera::worldToScreen(Vector2f p, Rect4f viewport) const
{
	const auto angle = getZAngle();
	Vector2f pos2d(pos.x, pos.y);
	return (p - pos2d).rotate(-angle) * zoom + viewport.getCenter();
}
