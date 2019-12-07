#include "console_window.h"

using namespace Halley;

ConsoleWindow::ConsoleWindow(UIFactory& ui)
	: UIWidget("console", {}, UISizer())
{
	console = std::make_shared<UIDebugConsole>("debugConsole", ui, controller);

	Logger::addSink(*this);
	ConsoleWindow::add(console, 1, Vector4f(8, 8, 8, 8));
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

void ConsoleWindow::update(Time t, bool moved)
{
	std::unique_lock<std::mutex> lock(mutex);
	decltype(buffer) buf2 = std::move(buffer);
	buffer.clear();
	lock.unlock();

	for (auto& b: buf2) {
		Colour4f col;
		switch (b.first) {
		case LoggerLevel::Info:
			col = Colour4f(1, 1, 1);
			break;
		case LoggerLevel::Dev:
			col = Colour4f(0.2, 0.4, 1);
			break;
		case LoggerLevel::Warning:
			col = Colour4f(1, 1, 0);
			break;
		case LoggerLevel::Error:
			col = Colour4f(1, 0, 0);
			break;
		}
		console->addLine(b.second, col);
	}
}
