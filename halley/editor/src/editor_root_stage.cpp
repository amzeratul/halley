#include "editor_root_stage.h"

using namespace Halley;

void EditorRootStage::init()
{
}

void EditorRootStage::onVariableUpdate(Time time)
{
}

void EditorRootStage::onRender(RenderContext& context) const
{
	context.bind([&] (Painter& painter)
	{
		painter.clear(Colour(0.2f, 0.2f, 0.2f));
	});
}
