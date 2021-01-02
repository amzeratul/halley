#include "task_details.h"
using namespace Halley;

TaskDetails::TaskDetails(UIFactory& factory)
	: UIWidget("taskDetails", {}, UISizer())
	, factory(factory)
{
	factory.loadUI(*this, "ui/halley/task_details");
	setMouseBlocker(false);
}

void TaskDetails::onMakeUI()
{
}

void TaskDetails::show(const TaskDisplay& display)
{
	if (taskDisplay != &display) {
		taskDisplay = &display;
		lastNumMessages = 0;
		lastStatus = EditorTaskStatus::WaitingToStart;
	}
	
	visible = true;
	showTime = 0.0;
	update(0, false);
}

void TaskDetails::hide()
{
	taskDisplay = nullptr;
	visible = false;
}

void TaskDetails::onMouseOver(Vector2f mousePos)
{
	showTime = 0.0;
}

bool TaskDetails::canInteractWithMouse() const
{
	return true;
}

void TaskDetails::update(Time t, bool moved)
{
	showTime += t;
	if (showTime >= 0.2) {
		hide();
	}
	if (taskDisplay) {
		const auto pos = taskDisplay->getPosition() + Vector2f(0.5f, 0.0f) * taskDisplay->getSize() + Vector2f(0, -10.0f);

		updateMessages();
		layout();

		setAnchor(UIAnchor(Vector2f(0.0f, 0.0f), Vector2f(0.5f, 1.0f), pos));
		alignAtAnchor();
	}
}

void TaskDetails::drawChildren(UIPainter& painter) const
{
	if (visible) {
		auto painter2 = painter.withAdjustedLayer(90);
		UIWidget::drawChildren(painter2);
	}
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

void TaskDetails::updateMessages()
{
	const auto& task = taskDisplay->getTask();

	if (task->getNumMessages() != lastNumMessages || task->getStatus() != lastStatus) {
		lastNumMessages = task->getNumMessages();
		lastStatus = task->getStatus();
		
		const auto msgs = lastStatus == EditorTaskStatus::Done && task->hasError() ? task->copyMessagesHead(5, LoggerLevel::Error) : task->copyMessagesTail(5);

		const auto& colourScheme = factory.getColourScheme();
		const auto& msgLabel = getWidgetAs<UILabel>("messages");
		const auto& textRenderer = msgLabel->getTextRenderer();
		
		StringUTF32 message;
		std::vector<ColourOverride> colours;
		bool first = true;
		for (const auto& msg: msgs) {
			colours.emplace_back(message.length(), getColour(*colourScheme, msg.first));
			if (!first) {
				message.push_back('\n');
			}
			first = false;
			message += textRenderer.split(msg.second, 590.0f);
		}
		
		msgLabel->setText(LocalisedString::fromUserString(String(message)));
		msgLabel->getTextRenderer().setColourOverride(colours);
	}
}
