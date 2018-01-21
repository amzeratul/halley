#include "winrt_system.h"
using namespace Halley;

void WinRTSystem::init()
{
	// TODO
	std::cout << "Hello WinRT world!" << std::endl;
}

void WinRTSystem::deInit()
{
	// TODO
}

String WinRTSystem::getResourcesBasePath(const String& gamePath) const
{
	// TODO
	return "";
}

std::unique_ptr<ResourceDataReader> WinRTSystem::getDataReader(String path, int64_t start, int64_t end)
{
	// TODO
	return {};
}

std::unique_ptr<GLContext> WinRTSystem::createGLContext()
{
	// Not supported
	return {};
}

std::shared_ptr<Window> WinRTSystem::createWindow(const WindowDefinition& window)
{
	// TODO
	return {};
}

void WinRTSystem::destroyWindow(std::shared_ptr<Window> window)
{
	// TODO
}

Vector2i WinRTSystem::getScreenSize(int n) const
{
	// TODO
	return Vector2i();
}

Rect4i WinRTSystem::getDisplayRect(int screen) const
{
	// TODO
	return Rect4i();
}

void WinRTSystem::showCursor(bool show)
{
	// TODO
}

Bytes WinRTSystem::getSaveData(SaveDataType type, const String& path)
{
	// TODO
	return {};
}

void WinRTSystem::setSaveData(SaveDataType type, const String& path, const Bytes& data)
{
	// TODO
}

std::vector<String> WinRTSystem::enumerateSaveData(SaveDataType type, const String& root)
{
	// TODO
	return {};
}

bool WinRTSystem::generateEvents(VideoAPI* video, InputAPI* input)
{
	// TODO
	return true;
}
