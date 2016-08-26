#include "editor_root_stage.h"
#include "console/console_window.h"
#include "tasks/assets/check_assets_task.h"
#include "halley_editor.h"
#include "ui/taskbar/taskbar.h"

using namespace Halley;

EditorRootStage::EditorRootStage(HalleyEditor& editor)
	: editor(editor)
	, tasks(std::make_unique<EditorTaskSet>())
{
}

EditorRootStage::~EditorRootStage()
{
}

void EditorRootStage::init()
{
	tasks->addTask(EditorTaskAnchor(std::make_unique<CheckAssetsTask>(editor.getProject())));
	taskBar = std::make_unique<TaskBar>(getResources());

	initSprites();
	//console = std::make_unique<ConsoleWindow>(getResources());
}

void EditorRootStage::onVariableUpdate(Time time)
{
	tasks->update(time);
	taskBar->update(tasks->getTasks(), time);

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

		// Background
		Sprite bg = background;
		bg.setTexRect(view).setSize(view.getSize()).draw(painter);
		halleyLogo.draw(painter);

		// Taskbar
		taskBar->draw(painter);

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
