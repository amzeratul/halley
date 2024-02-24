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

	updateLED(0.0);

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

			if (next.first > LoggerLevel::Info) {
				pendingLED.emplace_back(LEDState{ next.first, 0.05, 0.2 });
				idleLEDLevel = std::max(idleLEDLevel, next.first);
			}

			pendingStatus.pop_front();
		}
	}

	updateLED(t);

	background.setPosition(getPosition()).scaleTo(getSize());
	statusLED.setPosition(getPosition() + Vector2f(2, 2));
	statusText.setPosition(getPosition() + Vector2f(25, 3));
}

void StatusBar::draw(UIPainter& painter) const
{
	painter.draw(background);
	painter.draw(statusLED);
	auto p2 = painter.withClip(getRect().shrink(2));
	p2.draw(statusText);
}

void StatusBar::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0) {
		projectWindow.setPage(EditorTabs::Terminal);
	}
}

std::optional<MouseCursorMode> StatusBar::getMouseCursorMode() const
{
	return MouseCursorMode::Hand;
}

void StatusBar::updateLED(Time t)
{
	LoggerLevel curLevel = idleLEDLevel;

	if (!pendingLED.empty()) {
		auto& curState = pendingLED.front();
		if (curState.offTime > 0) {
			curState.offTime -= t;
			curLevel = LoggerLevel::Info;
		} else {
			curState.onTime -= t;
			curLevel = curState.level;
			if (curState.onTime < 0) {
				pendingLED.pop_front();
			}
		}
	}

	const auto& colourScheme = factory.getColourScheme();
	if (curLevel == LoggerLevel::Info) {
		statusLED.setColour(colourScheme->getColour("ui_text").withAlpha(0.2f));
	} else {
		statusLED.setColour(ConsoleWindow::getColour(*colourScheme, curLevel));
	}
}

void StatusBar::resetLED()
{
	idleLEDLevel = LoggerLevel::Info;
}

void StatusBar::log(LoggerLevel level, std::string_view msg)
{
	if (level != LoggerLevel::Dev) {
		std::unique_lock<std::mutex> lock(mutex);
		pendingStatus.emplace_back(level, String(msg));
	}
}
