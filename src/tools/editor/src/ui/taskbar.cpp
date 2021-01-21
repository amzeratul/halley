#include "taskbar.h"
#include "task_details.h"
#include "task_display.h"

using namespace Halley;

TaskBar::TaskBar(UIFactory& ui, EditorTaskSet& taskSet, const HalleyAPI& api)
	: UIWidget("taskBar", Vector2f(150.0f, 80.0), {}, Vector4f(160, 8, 10, 8))
	, factory(ui)
	, resources(ui.getResources())
	, taskSet(taskSet)
	, api(api)
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

TaskBar::~TaskBar()
{
	if (taskDetails) {
		taskDetails->destroy();
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
		t->layout();
	}

	// Update and decay
	for (size_t i = 0; i < tasks.size();) {
		const bool ok = tasks[i]->updateTask(time, static_cast<float>(i));
		if (ok) {
			++i;
		} else {
			tasks[i]->destroy();
			if (tasks[i].get() == taskDisplayHovered) {
				taskDisplayHovered = nullptr;
				if (taskDetails) {
					taskDetails->hide();
				}
			}
			tasks.erase(tasks.begin() + i);
		}
	}

	// Update bar size
	displaySize = lerp(displaySize, float(tasks.size()), static_cast<float>(6 * time));

	// Show display
	if (waitingToShowTaskDisplay) {
		waitingToShowTaskDisplay = false;
		if (taskDisplayHovered) {
			if (!taskDetails) {
				taskDetails = std::make_shared<TaskDetails>(factory, api.system->getClipboard());
				taskDetails->hide();
				getRoot()->addChild(taskDetails);
			}

			taskDetails->show(*taskDisplayHovered);
		}
	}
}

void TaskBar::draw(UIPainter& painter) const
{
	painter.draw(barSolid);
	painter.draw(barFade);
	painter.draw(halleyLogo);
}

void TaskBar::showTaskDetails(const TaskDisplay& taskDisplay)
{
	taskDisplayHovered = &taskDisplay;
	waitingToShowTaskDisplay = true;
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
		if (existingTask->getStatus() == EditorTaskStatus::Done && existingTask->getName() == task->getName()) {
			t->setTask(task);
			return t;
		}
	}

	auto taskDisplay = std::make_shared<TaskDisplay>(factory, task, *this);
	add(taskDisplay, 0, Vector4f(), UISizerAlignFlags::Centre);
	tasks.emplace_back(taskDisplay);
	
	return taskDisplay;
}
