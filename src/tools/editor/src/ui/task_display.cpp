#include "task_display.h"
using namespace Halley;


TaskDisplay::TaskDisplay(UIFactory& factory, std::shared_ptr<EditorTaskAnchor> task)
	: UIWidget("task", {}, UISizer())
	, factory(factory)
	, task(std::move(task))
{
	factory.loadUI(*this, "ui/halley/task");
}

void TaskDisplay::update(Time t, bool moved)
{
	const float maxTextWidth = std::max(100.0f, getSize().x - 22.0f);
	name->setText(LocalisedString::fromUserString(task->getName()));
	desc->setText(LocalisedString::fromUserString(task->getProgressLabel()));
	name->setMaxWidth(maxTextWidth);
	desc->setMaxWidth(maxTextWidth);

	const float progress = clamp(task->getProgress(), 0.0f, 1.0f);
	
	const auto& cs = factory.getColourScheme();
	const Colour col = task->hasError() ? cs->getColour("taskError") : (progress > 0.9999f ? cs->getColour("taskComplete") : cs->getColour("taskRunning"));
	
	bg->getSprite().setColour(col.multiplyAlpha(0.8f));
	bgFill->getSprite().setColour(col.multiplyAlpha(0.5f));
	bgFill->setMinSize(Vector2f((getSize().x - 12) * progress + 10.0f, 38.0f));
}

bool TaskDisplay::updateTask(Time time, float targetDisplaySlot)
{
	constexpr float displayTime = 1.5f;
	
	if (displaySlot < 0) {
		displaySlot = targetDisplaySlot;
	} else {
		displaySlot = lerp(displaySlot, targetDisplaySlot, static_cast<float>(6 * time));
	}

	if (task->getStatus() == EditorTaskStatus::Done) {
		progressDisplay = 1;
		completeTime += static_cast<float>(time);
		if (completeTime >= displayTime && !task->hasError()) {
			return false;
		}
	} else {
		progressDisplay = lerp(progressDisplay, task->getProgress(), static_cast<float>(10 * time));
	}
	
	return true;
}

void TaskDisplay::setTask(std::shared_ptr<EditorTaskAnchor> task)
{
	progressDisplay = 0;
	completeTime = 0;
	this->task = std::move(task);
}

void TaskDisplay::onMakeUI()
{
	name = getWidgetAs<UILabel>("name");
	name->setWordWrapped(false);
	desc = getWidgetAs<UILabel>("desc");
	desc->setWordWrapped(false);
	bg = getWidgetAs<UIImage>("bg");
	bgFill = getWidgetAs<UIImage>("bgFill");
}
