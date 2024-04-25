#include "halley/entity/services/screen_service.h"

using namespace Halley;

ScreenService::ScreenService(IScreenServiceInterface* interface)
	: interface(interface)
{
}

ScreenService::ScreenService(std::shared_ptr<IScreenServiceInterface> interface)
	: interface(interface.get())
	, interfacePtr(interface)
{
}

bool ScreenService::hasInterface() const
{
	return !!interface;
}

Vector2i ScreenService::getGameResolution() const
{
	return interface->getGameResolution();
}

Vector2i ScreenService::getScreenResolution() const
{
	return interface->getScreenResolution();
}

Vector2i ScreenService::getUIResolution() const
{
	return interface->getUIResolution();
}

float ScreenService::getZoomLevel() const
{
	return interface->getZoomLevel();
}

void ScreenService::setCameraPosition(Vector2f camPos)
{
	cameraPosition = camPos;
}

bool ScreenService::isVisible(const Rect4f& aabb) const
{
	return getCameraViewPort().overlaps(aabb);
}

Vector2f ScreenService::worldToScreen(Vector2f pos) const
{
	return (pos - cameraPosition) * getZoomLevel() + Vector2f(getScreenResolution() / 2);
}

Vector2f ScreenService::worldToUI(Vector2f pos) const
{
	return (pos - cameraPosition) + Vector2f(getUIResolution() / 2);
}

Vector2f ScreenService::screenToWorld(Vector2f pos) const
{
	return (pos - Vector2f(getScreenResolution() / 2)) / getZoomLevel() + cameraPosition;
}

Vector2f ScreenService::screenToUI(Vector2f pos) const
{
	return pos / getZoomLevel();
}

void ScreenService::setScreenGrabInterface(IScreenGrabInterface* interface)
{
	screenGrabInterface = interface;
}

Future<std::unique_ptr<Image>> ScreenService::requestScreenGrab(std::optional<Rect4i> rect, ScreenGrabMode mode)
{
	Expects(screenGrabInterface != nullptr);
	return screenGrabInterface->requestScreenGrab(rect, mode);
}

Future<std::unique_ptr<Image>> ScreenService::requestGlobalScreenGrab(Rect4i worldRect, ScreenGrabMode mode, float zoom)
{
	Expects(screenGrabInterface != nullptr);
	screenGrabMode = true;
	return screenGrabInterface->requestGlobalScreenGrab(worldRect, mode, zoom).then([this] (std::unique_ptr<Image> img) -> std::unique_ptr<Image>
	{
		screenGrabMode = false;
		return img;
	});
}

bool ScreenService::isScreenGrabMode() const
{
	return screenGrabMode;
}

std::shared_ptr<IScreenServiceInterface> ScreenService::getInterfacePointer()
{
	return interfacePtr;
}

Rect4f ScreenService::getCameraViewPort() const
{
	return getCameraViewPort(cameraPosition, getGameResolution());
}

Rect4f ScreenService::getCameraViewPort(Vector2f cameraPos, Vector2i gameRes)
{
	const auto screenSize = Vector2f(gameRes) * 0.5f;
	return Rect4f(-screenSize, screenSize) + cameraPos;
}
