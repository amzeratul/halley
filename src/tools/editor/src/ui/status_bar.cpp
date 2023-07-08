#include "status_bar.h"

#include "console_window.h"
#include "project_window.h"

using namespace Halley;

StatusBar::StatusBar(UIFactory& factory, ProjectWindow& projectWindow)
	: UIWidget("status_bar", Vector2f(24, 24), UISizer())
	, factory(factory)
	, projectWindow(projectWindow)
{
	const auto& colourScheme = factory.getColourScheme();

	statusText
		.setFont(factory.getResources().get<Font>("Ubuntu Regular"))
		.setSize(15);
	background
		.setImage(factory.getResources(), "halley_ui/ui_window.png")
		.setColour(colourScheme->getColour("ui_popupWindow"));
	statusLED
		.setImage(factory.getResources(), "halley_ui/ui_status_led.png");

	resetLED();

	setInteractWithMouse(true);

	Logger::addSink(*this);
}

StatusBar::~StatusBar()
{
	Logger::removeSink(*this);
}

void StatusBar::notifyConsoleOpen()
{
	resetLED();
}

void StatusBar::update(Time t, bool moved)
{
	{
		std::unique_lock<std::mutex> lock(mutex);
		if (!pendingStatus.empty()) {
			const auto& next = pendingStatus.front();
			const auto colour = ConsoleWindow::getColour(*factory.getColourScheme(), next.first);

			statusText.setText(LocalisedString::fromUserString(next.second));
			statusText.setColour(colour);
			statusLED.setColour(colour);

			pendingStatus.pop_front();
		}
	}

	background.setPosition(getPosition()).scaleTo(getSize());
	statusLED.setPosition(getPosition() + Vector2f(2, 2));
	statusText.setPosition(getPosition() + Vector2f(25, 3));
}

void StatusBar::draw(UIPainter& painter) const
{
	painter.draw(background);
	painter.draw(statusLED);
	auto p2 = painter.withClip(getRect().shrink(1));
	p2.draw(statusText);
}

void StatusBar::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0) {
		projectWindow.setPage(EditorTabs::Terminal);
	}
}

void StatusBar::resetLED()
{
	const auto& colourScheme = factory.getColourScheme();
	statusLED.setColour(colourScheme->getColour("ui_text").withAlpha(0.25f));
}

void StatusBar::log(LoggerLevel level, std::string_view msg)
{
	if (level != LoggerLevel::Dev) {
		std::unique_lock<std::mutex> lock(mutex);
		pendingStatus.emplace_back(level, String(msg));
	}
}
