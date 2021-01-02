#include "taskbar.h"
#include "halley/tools/tasks/editor_task_set.h"

using namespace Halley;

TaskBar::TaskBar(UIFactory& ui, EditorTaskSet& taskSet)
	: UIWidget("taskBar", Vector2f(150.0f, 80.0), {}, Vector4f(160, 8, 10, 8))
	, factory(ui)
	, resources(ui.getResources())
	, taskSet(taskSet)
{
	{
		const auto col = ui.getColourScheme()->getColour("logo");
		halleyLogo = Sprite()
			.setImage(resources, "halley/halley_logo_dist.png", "Halley/DistanceFieldSprite")
			.setPivot(Vector2f(0.5f, 0.5f))
			.setScale(Vector2f(0.5f, 0.5f))
			.setColour(col);
		halleyLogo.getMutableMaterial()
			.set("u_smoothness", 16.0f)
			.set("u_outline", 0.0f)
			.set("u_outlineColour", col);
	}

	{
		const auto col = ui.getColourScheme()->getColour("taskbarLogoBackground");
		barSolid = Sprite().setMaterial(resources, "Halley/SolidColour").setSize(Vector2f(1, 1)).setColour(col);
		barFade = Sprite().setImage(resources, "fade_right.png").setColour(col);
	}
}

void TaskBar::update(Time time, bool moved)
{
	auto taskData = taskSet.getTasks();

	// Ensure that all tasks have a display associated
	for (auto& t : taskData) {
		if (t->isVisible() && t->getStatus() == EditorTaskStatus::Started) {
			getDisplayFor(t);
		}
	}

	// Setup for tasks
	const Vector2f anchor = getPosition() + Vector2f(0, 80.0f);
	const Vector2f baseDrawPos = getPosition() + Vector2f(170.0f, 20.0f);
	const Vector2f size = Vector2f(std::min(400.0f, getSize().x / std::max(1.0f, displaySize)), 40);
	const float totalLen = baseDrawPos.x + (displaySize * size.x) + 10.0f;

	// Draw logo
	barSolid.setScale(Vector2f(totalLen, 32)).setPos(anchor + Vector2f(0, -56));
	barFade.setPos(anchor + Vector2f(totalLen, -56));
	halleyLogo.setPos(anchor + Vector2f(80, -41));

	for (const auto& t : tasks) {
		const Vector2f drawPos = baseDrawPos + Vector2f((size.x + 20) * t->getDisplaySlot(), 0);
		t->setPosition(drawPos);
		t->setMinSize(size);
	}

	// Update and decay
	for (size_t i = 0; i < tasks.size();) {
		const bool ok = tasks[i]->updateTask(time, static_cast<float>(i));
		if (ok) {
			++i;
		} else {
			tasks[i]->destroy();
			tasks.erase(tasks.begin() + i);
		}
	}

	// Update bar size
	displaySize = lerp(displaySize, float(tasks.size()), static_cast<float>(6 * time));
}

void TaskBar::draw(UIPainter& painter) const
{
	painter.draw(barSolid);
	painter.draw(barFade);
	painter.draw(halleyLogo);
}

std::shared_ptr<TaskDisplay> TaskBar::getDisplayFor(const std::shared_ptr<EditorTaskAnchor>& task)
{
	for (auto& t: tasks) {
		const auto& existingTask = t->getTask();
		
		// Already assigned one!
		if (existingTask == task) {
			return t;
		}

		// Replace error:
		if (existingTask->hasError() && existingTask->getName() == task->getName()) {
			t->setTask(task);
			return t;
		}
	}

	auto taskDisplay = std::make_shared<TaskDisplay>(factory, task);
	add(taskDisplay, 0, Vector4f(), UISizerAlignFlags::Centre);
	tasks.emplace_back(taskDisplay);
	
	return taskDisplay;
}

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
