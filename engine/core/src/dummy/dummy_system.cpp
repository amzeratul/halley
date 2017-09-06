#include "dummy_system.h"

using namespace Halley;

String DummySystemAPI::getResourcesBasePath(const String& gamePath) const
{
	return gamePath;
}

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

void DummySystemAPI::init()
{
}

void DummySystemAPI::deInit()
{
}

Bytes DummySystemAPI::getSaveData(SaveDataType type, const String& path)
{
	return Bytes();
}

void DummySystemAPI::setSaveData(SaveDataType type, const String& path, const Bytes& data)
{
}

std::vector<String> DummySystemAPI::enumerateSaveData(SaveDataType type, const String& root)
{
	return {};
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
