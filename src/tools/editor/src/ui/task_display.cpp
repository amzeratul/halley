#include "task_display.h"

#include "taskbar.h"
using namespace Halley;


TaskDisplay::TaskDisplay(UIFactory& factory, std::shared_ptr<EditorTaskAnchor> task, TaskBar& taskBar)
	: UIWidget("task", {}, UISizer())
	, factory(factory)
	, task(std::move(task))
	, taskBar(taskBar)
{
	factory.loadUI(*this, "ui/halley/task");
}

void TaskDisplay::update(Time t, bool moved)
{
	const float maxTextWidth = std::max(100.0f, getSize().x - 22.0f);
	name->getTextRenderer().setColour(nameCol.multiplyAlpha(opacity));
	desc->getTextRenderer().setColour(descCol.multiplyAlpha(opacity));
	name->setText(LocalisedString::fromUserString(task->getName()));
	desc->setText(LocalisedString::fromUserString(task->getProgressLabel()));
	name->setMaxWidth(maxTextWidth);
	desc->setMaxWidth(maxTextWidth);

	const float progress = clamp(task->getProgress(), 0.0f, 1.0f);
	
	const auto& cs = factory.getColourScheme();
	const Colour col = task->hasError() ? cs->getColour("taskError") : (progress > 0.9999f ? cs->getColour("taskComplete") : cs->getColour("taskRunning"));
	
	bg->getSprite().setColour(col.multiplyAlpha(0.8f * opacity));
	bgFill->getSprite().setColour(col.multiplyAlpha(0.5f * opacity));
	bgFill->setMinSize(Vector2f((getSize().x - 12) * progress + 10.0f, 38.0f));
}

bool TaskDisplay::updateTask(Time time, float targetDisplaySlot)
{
	constexpr float displayTime = 1.0f;
	
	if (displaySlot < 0) {
		displaySlot = targetDisplaySlot;
	} else {
		displaySlot = lerp(displaySlot, targetDisplaySlot, static_cast<float>(6 * time));
	}

	bool visible = true;

	if (task->getStatus() == EditorTaskStatus::Done) {
		progressDisplay = 1;
		completeTime += static_cast<float>(time);
		if (completeTime >= displayTime && !task->hasError()) {
			visible = false;
		}
	} else {
		progressDisplay = lerp(progressDisplay, task->getProgress(), static_cast<float>(10 * time));
	}

	opacity = advance(opacity, visible ? 1.0f : 0.0f, static_cast<float>(time) * 2.0f);
	
	return visible || opacity > 0.0001f;
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
	nameCol = name->getTextRenderer().getColour();
	desc = getWidgetAs<UILabel>("desc");
	desc->setWordWrapped(false);
	descCol = desc->getTextRenderer().getColour();
	bg = getWidgetAs<UIImage>("bg");
	bgFill = getWidgetAs<UIImage>("bgFill");
}

void TaskDisplay::onMouseOver(Vector2f mousePos)
{
	if (task->getNumMessages() > 0) {
		taskBar.showTaskDetails(*this);
	}
}

bool TaskDisplay::canInteractWithMouse() const
{
	return true;
}
