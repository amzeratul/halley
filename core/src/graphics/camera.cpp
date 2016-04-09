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

#include "camera.h"

using namespace Halley;


static std::shared_ptr<Camera> boundCamera;


void Halley::Camera::bind(std::shared_ptr<Camera> cam, RenderTarget& target)
{
	resetBind();
	boundCamera = cam;
	cam->doBind(target);
}

void Halley::Camera::bindScreen(RenderTarget& target)
{
	bind(std::make_shared<Camera>(), target);
}

void Halley::Camera::resetBind()
{
	boundCamera.reset();
}

void Halley::Camera::rebind(RenderTarget& target)
{
	if (boundCamera) {
		boundCamera->doBind(target);
	}
}

std::shared_ptr<Camera> Halley::Camera::getCurrentCamera()
{
	return boundCamera;
}


Camera::Camera()
	: zoom(1.0f)
{
	area = Vector2f(1280, 720);
	pos = 0.5*area;
	relativeViewPort = Rect4f(0, 0, 1, 1);
}


Camera::Camera(Vector2f _pos, Vector2f _area, Angle1f _angle)
	: pos(_pos)
	, area(_area)
	, angle(_angle)
	, zoom(1.0f)
{
	relativeViewPort = Rect4f(0, 0, 1, 1);
}


void Camera::setPosition(Vector2f _pos)
{
	pos = _pos;
}


void Camera::setViewArea(Vector2f _area)
{
	area = _area;
}


void Camera::setAngle(Angle1f _angle)
{
	angle = _angle;
}


void Camera::setZoom(float _zoom)
{
	zoom = _zoom;
}


void Camera::doBind(RenderTarget& target)
{
	Debug::trace("Camera::doBind begin");
	prepareViewPort(target);

	float w = area.x;
	float h = area.y;

	// Setup projection
	projection = Matrix4f::makeOrtho2D(-w/2, w/2, -h/2, h/2, -1000, 1000);

	// Camera properties
	if (zoom != 1.0f) {
		projection.scale2D(zoom, zoom);
	}
	if (angle.getRadians() != 0) {
		projection.rotateZ(-angle);
	}
	if (pos != Vector2f()) {
		projection.translate2D(-pos.x, -pos.y);
	}
	
	// FIXED PIPELINE
	/*
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMultMatrixf(projection.getElements());
	*/
	Debug::trace("Camera::doBind end");
}

void Halley::Camera::setRelativeViewPort(Vector2f origin, Vector2f ws)
{
	// Setup viewport
	relativeViewPort = Rect4f(origin, origin + ws);
}

void Halley::Camera::prepareViewPort(RenderTarget& target)
{
	/*
	// TODO
	Vector2f origin = target.getOrigin();
	Vector2f ws = target.getSize();
	int vx = int(origin.x + ws.x * relativeViewPort.getX());
	int vy = int(origin.y + ws.y * relativeViewPort.getY());
	int vw = int(ws.x * relativeViewPort.getWidth());
	int vh = int(ws.y * relativeViewPort.getHeight());
	
	bool enableScissor = vx > 0 || vy > 0 || vx + vw < int(ws.x) || vy + vh < int(ws.y);
	GLUtils utils;
	utils.setViewPort(Rect4i(vx, vy, vw, vh), enableScissor);
	*/
}

Halley::Rect4f Halley::Camera::getViewRect() const
{
	Vector2f area = getViewArea();
	return Rect4f(getPosition() - area / 2, area.x, area.y);
}

Vector2f Camera::screenToWorld(Vector2f p, Rect4f viewport) const
{
	// x and y relative to viewport, from -0.5 to 0.5
	Vector2f p2;
	p2.x = (p.x - viewport.getX()) / viewport.getWidth() - 0.5f;
	p2.y = -((p.y - viewport.getY()) / viewport.getHeight() - 0.5f);

	// Apply area/zoom
	p2 = p2 * area / zoom;

	// Rotate
	p2 = p2.rotate(angle);

	return p2 + pos;
}

Vector2f Camera::worldToScreen(Vector2f p, Rect4f) const
{
	// TODO
	return p;
}
