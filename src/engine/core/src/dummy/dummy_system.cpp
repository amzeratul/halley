#include "dummy_system.h"

using namespace Halley;

std::unique_ptr<ResourceDataReader> DummySystemAPI::getDataReader(String path, int64_t start, int64_t end)
{
	return {};
}

std::unique_ptr<GLContext> DummySystemAPI::createGLContext()
{
	return {};
}

std::shared_ptr<Window> DummySystemAPI::createWindow(const WindowDefinition& window)
{
	return {};
}

void DummySystemAPI::destroyWindow(std::shared_ptr<Window> window)
{
}

Vector2i DummySystemAPI::getScreenSize(int n) const
{
	return Vector2i(1280, 720);
}

Rect4i DummySystemAPI::getDisplayRect(int screen) const
{
	return Rect4i({}, Vector2i(1280, 720));
}

void DummySystemAPI::showCursor(bool show)
{
}

bool DummySystemAPI::generateEvents(VideoAPI* video, InputAPI* input)
{
	return true;
}

std::shared_ptr<ISaveData> DummySystemAPI::getStorageContainer(SaveDataType type, const String& containerName)
{
	return std::make_shared<DummySaveData>();
}

bool DummySaveData::isReady() const
{
	return true;
}

Bytes DummySaveData::getData(const String& path)
{
	return {};
}

std::vector<String> DummySaveData::enumerate(const String& root)
{
	return {};
}

void DummySaveData::setData(const String& path, const Bytes& data, bool commit)
{
}

void DummySaveData::commit()
{
}

void DummySystemAPI::init()
{
}

void DummySystemAPI::deInit()
{
}

Path DummySystemAPI::getAssetsPath(const Path& gamePath) const
{
	return gamePath;
}

Path DummySystemAPI::getUnpackedAssetsPath(const Path& gamePath) const
{
	return gamePath;
}


DummyWindow::DummyWindow(const WindowDefinition& definition) 
	: definition(definition)
{}

void DummyWindow::update(const WindowDefinition& definition) {}

void DummyWindow::show() {}

void DummyWindow::hide() {}

void DummyWindow::setVsync(bool vsync) {}

void DummyWindow::swap() {}

Rect4i DummyWindow::getWindowRect() const
{
	return Rect4i({}, definition.getSize());
}

const WindowDefinition& DummyWindow::getDefinition() const
{
	return definition;
}
