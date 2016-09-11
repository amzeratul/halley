#include "taskbar.h"

using namespace Halley;

TaskBar::TaskBar(Resources& resources)
{
	{
		taskMaterial = std::make_shared<Material>(resources.get<MaterialDefinition>("distance_field_sprite.yaml"));
		auto& mat = *taskMaterial;
		mat["tex0"] = resources.get<Texture>("round_rect.png");
		mat["u_smoothness"] = 0.1f;
		mat["u_outline"] = 0.4f;

		taskSprite = Sprite()
			.setSize(Vector2f(64, 64))
			.setTexRect(Rect4f(0, 0, 1, 1));
	}

	{
		auto col = Colour4f(0.9882f, 0.15686f, 0.27843f, 1);
		halleyLogo = Sprite()
			.setImage(resources, "halley_logo_dist.png", "distance_field_sprite.yaml")
			.setPivot(Vector2f(0.5f, 0.5f))
			.setColour(col)
			.setScale(Vector2f(0.5f, 0.5f));
		auto& mat = halleyLogo.getMaterial();
		mat["u_smoothness"] = 0.4f;
		mat["u_outline"] = 0.0f;
		mat["u_outlineColour"] = col;
	}

	{
		Colour4f col(0.12f, 0.12f, 0.12f);
		barSolid = Sprite().setMaterial(resources, "solid_colour.yaml").setSize(Vector2f(1, 1)).setColour(col);
		barFade = Sprite().setImage(resources, "fade_right.png").setColour(col);
	}

	font = resources.get<Font>("ubuntub.font");
}

void TaskBar::update(const std::list<EditorTaskAnchor>& taskData, Time time)
{
	// Flag them all as not running, so anything not found is accurately set as not running
	for (auto& t : tasks) {
		t.running = false;
		t.progress = 1;
		t.subLabel = "";
	}

	// Copy data
	for (auto& t : taskData) {
		if (t.isVisible() && t.getStatus() == EditorTaskStatus::Started) {
			auto& display = getDisplayForId(t.getId());
			display.label = t.getName();
			display.subLabel = t.getProgressLabel();
			display.progress = t.getProgress();
			display.running = true;
		}
	}

	// Update and decay
	for (size_t i = 0; i < tasks.size();) {
		auto& t = tasks[i];

		float targetDisplaySlot = float(i);
		if (t.displaySlot < 0) {
			t.displaySlot = targetDisplaySlot;
		} else {
			t.displaySlot = lerp(t.displaySlot, targetDisplaySlot, static_cast<float>(6 * time));
		}

		if (t.running) {
			t.progressDisplay = lerp(t.progressDisplay, t.progress, static_cast<float>(10 * time));
		} else {
			t.progressDisplay = 1;
			t.completeTime += static_cast<float>(time);
			if (t.completeTime >= 1.5f) {
				tasks.erase(tasks.begin() + i);
				continue;
			}
		}
		++i;
	}

	// Update bar size
	displaySize = lerp(displaySize, float(tasks.size()), static_cast<float>(6 * time));
}

void TaskBar::draw(Painter& painter)
{
	// Setup for tasks
	auto view = painter.getViewPort();
	Vector2f anchor = Vector2f(view.getBottomLeft());
	Vector2f baseDrawPos = anchor + Vector2f(150, -72);
	Vector2f size = Vector2f(std::min(400.0f, (view.getWidth() - baseDrawPos.x - 150) / std::max(1.0f, displaySize)), 40);
	float totalLen = baseDrawPos.x + (displaySize * size.x) + 10.0f;

	// Draw logo
	barSolid.setScale(Vector2f(totalLen, 32)).setPos(anchor + Vector2f(0, -56)).draw(painter);
	barFade.setPos(anchor + Vector2f(totalLen, -56)).draw(painter);
	halleyLogo.setPos(anchor + Vector2f(80, -41)).draw(painter);

	if (tasks.empty()) {
		// Nothing to do here!
		return;
	}

	// Setup text renderer
	TextRenderer text;
	text.setFont(font).setOffset(Vector2f(0, 0)).setColour(Colour(1, 1, 1)).setOutline(1.0f).setOutlineColour(Colour(0, 0, 0, 0.35f));

	// Draw tasks
	for (auto& t : tasks) {
		Vector2f drawPos = baseDrawPos + Vector2f((size.x + 20) * t.displaySlot, 0);

		//Colour col = t.progress > 0.9999f ? Colour(0.16f, 0.69f, 0.34f) : Colour(0.96f, 0.78f, 0.0f);
		Colour col = t.progress > 0.9999f ? Colour(0.16f, 0.69f, 0.34f) : Colour(0.18f, 0.53f, 0.87f);
		(*t.material)["u_outlineColour"] = col;

		// Background
		taskSprite.clone()
			.setMaterial(t.material)
			.setPos(drawPos)
			.setScale((size + Vector2f(24, 24)) / Vector2f(64, 64))
			.setColour(Colour4f(0.15f, 0.15f, 0.19f))
			.drawSliced(painter, Vector4f(0.45f, 0.45f, 0.45f, 0.45f));

		// Progress
		taskSprite.clone()
			.setMaterial(t.material)
			.setPos(drawPos)
			.setScale((size * Vector2f(t.progressDisplay, 1) + Vector2f(24, 24)) / Vector2f(64, 64))
			.setColour(col)
			.drawSliced(painter, Vector4f(0.45f, 0.45f, 0.45f, 0.45f));

		// Text
		text.setSize(14).setText(t.label).draw(painter, drawPos + Vector2f(24, 12));
		text.setSize(12).setText(t.subLabel).draw(painter, drawPos + Vector2f(24, 30));
	}
}

TaskBar::TaskDisplay& TaskBar::getDisplayForId(int id)
{
	for (auto& t : tasks) {
		if (t.id == id) {
			return t;
		}
	}

	tasks.push_back(TaskDisplay());
	TaskDisplay& display = tasks.back();

	display.material = std::make_shared<Material>(*taskMaterial);
	display.id = id;
	return display;
}
