#pragma once

#include "graph_gizmo_ui.h"
#include "halley/graph/base_graph_gizmo.h"

namespace Halley {
	class RenderGraphEditor;

	class RenderGraphGizmo : public BaseGraphGizmo {
	public:
		RenderGraphGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, Resources& resources);

	protected:
		std::unique_ptr<BaseGraphNode> makeNode(const ConfigNode& node) override;
		std::shared_ptr<BaseGraphRenderer> makeRenderer(Resources& resources, float baseZoom) override;
	};

	class RenderGraphGizmoUI : public GraphGizmoUI {
	public:
		RenderGraphGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<InputKeyboard> keyboard, std::shared_ptr<IClipboard> clipboard, RenderGraphEditor& graphEditor);

		void load(BaseGraph& graph) override;

	private:
		RenderGraphGizmo* renderGraphGizmo = nullptr;
	};

	class RenderGraphRenderer : public BaseGraphRenderer {
	public:
		RenderGraphRenderer(Resources& resources, float nativeZoom);

		const IGraphNodeType* tryGetNodeType(const String& typeId) const override;
	};


	class RenderGraphNodeType : public IGraphNodeType {
	public:
		String getId() const override;
		String getName() const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
	};


}
