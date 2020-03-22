#include "win32_system.h"

#include "win32_data_reader.h"
#include "win32_window.h"
using namespace Halley;

void Win32System::init()
{}

void Win32System::deInit()
{}

Path Win32System::getAssetsPath(const Path& gamePath) const
{
	return gamePath / ".." / "assets";
}

Path Win32System::getUnpackedAssetsPath(const Path& gamePath) const
{
	return gamePath / ".." / "assets_unpacked";
}

std::unique_ptr<ResourceDataReader> Win32System::getDataReader(String path, int64_t start, int64_t end)
{
	return std::make_unique<Win32DataReader>(std::move(path), start, end);
}

std::shared_ptr<ISaveData> Win32System::getStorageContainer(SaveDataType type, const String& containerName)
{
	// TODO
	return {};
}

std::unique_ptr<GLContext> Win32System::createGLContext()
{
	// TODO
	return {};
}

std::shared_ptr<Window> Win32System::createWindow(const WindowDefinition& definition)
{
	return std::make_shared<Win32Window>(definition);
}

void Win32System::destroyWindow(std::shared_ptr<Window> window)
{
	std::static_pointer_cast<Win32Window>(window)->destroy();
}

Vector2i Win32System::getScreenSize(int n) const
{
	// TODO
	return Vector2i(1920, 1080);
}

Rect4i Win32System::getDisplayRect(int screen) const
{
	// TODO
	return Rect4i(Vector2i(), Vector2i(1920, 1080));
}

void Win32System::showCursor(bool show)
{
	// TODO
}

bool Win32System::generateEvents(VideoAPI* video, InputAPI* input)
{
	// TODO
	return true;
}
