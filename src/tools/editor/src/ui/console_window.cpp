#include "console_window.h"

using namespace Halley;

ConsoleWindow::ConsoleWindow(UIFactory& ui)
	: UIWidget("console", {}, UISizer())
	, factory(ui)
{
	controller = std::make_shared<UIDebugConsoleController>();
	console = std::make_shared<UIDebugConsole>("debugConsole", ui, controller);

	Logger::addSink(*this);
	ConsoleWindow::add(console, 1);
	ConsoleWindow::log(LoggerLevel::Info, "Welcome to the Halley Game Engine Editor.");
}

ConsoleWindow::~ConsoleWindow()
{
	Logger::removeSink(*this);
}

void ConsoleWindow::log(LoggerLevel level, const String& msg)
{
	std::unique_lock<std::mutex> lock(mutex);
	buffer.emplace_back(level, msg);
}

static Colour4f getColour(const UIColourScheme& colourScheme, LoggerLevel level)
{
	switch (level) {
	case LoggerLevel::Info:
		return colourScheme.getColour("ui_logInfoText");
	case LoggerLevel::Warning:
		return colourScheme.getColour("ui_logWarningText");
	case LoggerLevel::Error:
		return colourScheme.getColour("ui_logErrorText");
	case LoggerLevel::Dev:
		return colourScheme.getColour("ui_logDevText");
	}
	return colourScheme.getColour("ui_text");
}

void ConsoleWindow::update(Time t, bool moved)
{
	std::unique_lock<std::mutex> lock(mutex);
	decltype(buffer) buf2 = std::move(buffer);
	buffer.clear();
	lock.unlock();

	const auto& colourScheme = *factory.getColourScheme();

	for (auto& b: buf2) {
		console->addLine(b.second, getColour(colourScheme, b.first));
	}
}
