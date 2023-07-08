#include "status_bar.h"

#include "console_window.h"

using namespace Halley;

StatusBar::StatusBar(UIFactory& factory, ProjectWindow& projectWindow)
	: UIWidget("status_bar", Vector2f(24, 24), UISizer())
	, factory(factory)
	, projectWindow(projectWindow)
{
	const auto& colourScheme = factory.getColourScheme();

	statusText
		.setFont(factory.getResources().get<Font>("Ubuntu Regular"))
		.setSize(14);
	background
		.setImage(factory.getResources(), "halley_ui/ui_window.png")
		.setColour(colourScheme->getColour("ui_popupWindow"));

	Logger::addSink(*this);
}

StatusBar::~StatusBar()
{
	Logger::removeSink(*this);
}

void StatusBar::update(Time t, bool moved)
{
	{
		std::unique_lock<std::mutex> lock(mutex);
		if (nextStatus) {
			const auto& colourScheme = factory.getColourScheme();
			statusText.setText(LocalisedString::fromUserString(nextStatus->second));
			statusText.setColour(ConsoleWindow::getColour(*colourScheme, nextStatus->first));
		}
		nextStatus = {};
	}

	background.setPosition(getPosition()).scaleTo(getSize());
	statusText.setPosition(getPosition() + Vector2f(4, 4));
}

void StatusBar::draw(UIPainter& painter) const
{
	painter.draw(background);
	auto p2 = painter.withClip(getRect().shrink(1));
	p2.draw(statusText);
}

void StatusBar::log(LoggerLevel level, std::string_view msg)
{
	if (level != LoggerLevel::Dev) {
		std::unique_lock<std::mutex> lock(mutex);
		nextStatus = std::pair(level, String(msg));
	}
}
