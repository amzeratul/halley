#include "render_graph_gizmo_ui.h"
#include "render_graph_editor.h"

using namespace Halley;

RenderGraphGizmo::RenderGraphGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, Resources& resources)
	: BaseGraphGizmo(factory, entityEditorFactory, resources)
{
}

std::unique_ptr<BaseGraphNode> RenderGraphGizmo::makeNode(const ConfigNode& node)
{
	// TODO
	return {};
}

std::shared_ptr<BaseGraphRenderer> RenderGraphGizmo::makeRenderer(Resources& resources, float baseZoom)
{
	// TODO
	return {};
}

RenderGraphGizmoUI::RenderGraphGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<InputKeyboard> keyboard, std::shared_ptr<IClipboard> clipboard, RenderGraphEditor& graphEditor)
	: GraphGizmoUI(std::move(keyboard), std::move(clipboard), graphEditor, std::make_unique<RenderGraphGizmo>(factory, entityEditorFactory, resources))
{
	renderGraphGizmo = dynamic_cast<RenderGraphGizmo*>(gizmo.get());
}

void RenderGraphGizmoUI::load(BaseGraph& graph)
{
	// TODO
}
