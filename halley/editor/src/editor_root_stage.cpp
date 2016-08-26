#include "editor_root_stage.h"
#include "console/console_window.h"

using namespace Halley;

EditorRootStage::EditorRootStage(HalleyEditor& editor)
	: editor(editor)
{
}

EditorRootStage::~EditorRootStage()
{
}

void EditorRootStage::init()
{
	initSprites();
	//console = std::make_unique<ConsoleWindow>(getResources());
}

void EditorRootStage::onVariableUpdate(Time time)
{
	if (console) {
		console->update(*getInputAPI().getKeyboard());
	}

	halleyLogo.setPos(Vector2f(getVideoAPI().getWindow().getSize() / 2));
}

void EditorRootStage::onRender(RenderContext& context) const
{
	context.bind([&](Painter& painter)
	{
		auto view = Rect4f(painter.getViewPort());

		Sprite bg = background;
		bg.setTexRect(view).setSize(view.getSize()).draw(painter);
		halleyLogo.draw(painter);

		if (console) {
			console->draw(painter, Rect4f(view.getTopLeft() + Vector2f(64, 64), view.getBottomRight() - Vector2f(64, 64)));
		}
	});
}

void EditorRootStage::initSprites()
{
	{
		// Background
		{
			auto matRaw = std::make_shared<Material>(getResource<MaterialDefinition>("scanlines.yaml"));
			auto& mat = *matRaw;
			mat["u_col0"] = Colour4f(0.08f);
			mat["u_col1"] = Colour4f(0.07f);
			mat["u_distance"] = 6.0f;
			background = Sprite().setMaterial(matRaw).setPos(Vector2f(0, 0));
		}
	}

	{
		// Halley logo
		auto col = Colour4f(0.065f);
		//auto col = Colour4f(0.9882f, 0.15686f, 0.27843f, 1);
		halleyLogo = Sprite()
			.setImage(getResources(), "halley_logo_dist.png", "distance_field_sprite.yaml")
			.setPivot(Vector2f(0.5f, 0.5f))
			.setColour(col)
			.setScale(Vector2f(2, 2))
			.setPos(Vector2f(640, 360));
		auto& mat = halleyLogo.getMaterial();
		mat["u_smoothness"] = 0.1f;
		mat["u_outline"] = 0.0f;
		mat["u_outlineColour"] = col;
	}
}

void EditorRootStage::updateTasks(Time time)
{
	for (size_t i = 0; i < tasks.size(); ) {
		tasks[i].update(static_cast<float>(time));
		if (tasks[i].getStatus() == EditorTaskStatus::Done) {
			auto newTasks = std::move(tasks[i].getContinuations());
			std::move(newTasks.begin(), newTasks.end(), tasks.end());
			tasks.erase(tasks.begin() + i);
		} else {
			++i;
		}
	}
}
