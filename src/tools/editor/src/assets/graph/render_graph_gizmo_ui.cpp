#include "render_graph_gizmo_ui.h"
#include "render_graph_editor.h"

using namespace Halley;

RenderGraphGizmo::RenderGraphGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, Resources& resources)
	: BaseGraphGizmo(factory, entityEditorFactory, resources)
{
}

std::unique_ptr<BaseGraphNode> RenderGraphGizmo::makeNode(const ConfigNode& node)
{
	return std::make_unique<RenderGraphNode2>(node);
}

std::shared_ptr<BaseGraphRenderer> RenderGraphGizmo::makeRenderer(Resources& resources, float baseZoom)
{
	return std::make_shared<RenderGraphRenderer>(resources, baseZoom);
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

RenderGraphRenderer::RenderGraphRenderer(Resources& resources, float nativeZoom)
	: BaseGraphRenderer(resources, nativeZoom)
{}

const IGraphNodeType* RenderGraphRenderer::tryGetNodeType(const String& typeId) const
{
	// TODO
	static RenderGraphNodeType type;
	return &type;
}

String RenderGraphNodeType::getId() const
{
	// TODO
	return "todo";
}

String RenderGraphNodeType::getName() const
{
	// TODO
	return "TODO";
}

gsl::span<const IGraphNodeType::PinType> RenderGraphNodeType::getPinConfiguration(const BaseGraphNode& node) const
{
	// TODO
	return {};
}
