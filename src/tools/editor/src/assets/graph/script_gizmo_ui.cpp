#include "script_gizmo_ui.h"
using namespace Halley;

ScriptGizmoUI::ScriptGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes)
	: UIWidget("scriptGizmoUI", Vector2f(), {})
	, factory(factory)
	, resources(resources)
	, gizmo(factory, entityEditorFactory, nullptr, scriptNodeTypes)
{
	gizmo.setModifiedCallback([=] ()
	{
		onModified();
	});
}

void ScriptGizmoUI::onAddedToRoot(UIRoot& root)
{
	gizmo.setUIRoot(root);
}

void ScriptGizmoUI::load(ScriptGraph& graph)
{
	gizmo.setGraph(&graph);
}

void ScriptGizmoUI::update(Time time, bool moved)
{
	SceneEditorInputState inputState;
	gizmo.update(time, resources, inputState);
}

void ScriptGizmoUI::draw(UIPainter& painter) const
{
	painter.draw([=] (Painter& p)
	{
		gizmo.draw(p);
	});
}

void ScriptGizmoUI::onModified()
{
}
