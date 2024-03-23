#pragma once

#include "graph_gizmo_ui.h"
#include "halley/graph/base_graph_gizmo.h"

namespace Halley {
	class RenderGraphEditor;

	class RenderGraphGizmo : public BaseGraphGizmo {
	public:
		RenderGraphGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, Resources& resources, std::shared_ptr<GraphNodeTypeCollection> nodeTypes);

	protected:
		std::unique_ptr<BaseGraphNode> makeNode(const ConfigNode& node) override;
	};

	class RenderGraphGizmoUI : public GraphGizmoUI {
	public:
		RenderGraphGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<InputKeyboard> keyboard, std::shared_ptr<IClipboard> clipboard, RenderGraphEditor& graphEditor);

		void load(BaseGraph& graph) override;

	private:
		RenderGraphGizmo* renderGraphGizmo = nullptr;

		static std::shared_ptr<GraphNodeTypeCollection> makeRenderGraphTypes();
	};
	

	class RenderGraphNodeType : public IGraphNodeType {
	public:
		String getId() const override;
		String getName() const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
	};


}
