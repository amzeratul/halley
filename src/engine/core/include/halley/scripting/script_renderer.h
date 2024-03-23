#pragma once
#include "halley/graph/base_graph_type.h"
#include "halley/graph/base_graph_renderer.h"
#include "halley/graphics/sprite/sprite.h"
#include "halley/maths/vector2.h"

namespace Halley {
	class IScriptNodeType;
	class ScriptNodeTypeCollection;
	class World;
	class ScriptGraphNode;
	class ScriptGraph;
	class ScriptState;

	class ScriptRenderer : public BaseGraphRenderer {
	public:
		ScriptRenderer(Resources& resources, const GraphNodeTypeCollection& nodeTypeCollection, float nativeZoom);
		
		void setState(const ScriptState* scriptState);
		void setDebugDisplayData(HashMap<int, String> values) override;

	protected:
		bool isDimmed(GraphNodePinType type) const override;
		GraphPinSide getSide(GraphNodePinType pinType) const override;
		Colour4f getPinColour(GraphNodePinType pinType) const override;

	private:
		const ScriptState* state = nullptr;

		Sprite destructorBg;
		Sprite destructorIcon;
		HashMap<int, String> debugDisplayValues;

		void drawNodeBackground(Painter& painter, Vector2f basePos, const BaseGraphNode& node, float curZoom, float posScale, NodeDrawMode drawMode) override;
		std::pair<String, LabelType> getLabel(const IGraphNodeType& nodeType, const BaseGraphNode& node) const;

		NodeDrawMode getNodeDrawMode(GraphNodeId nodeId) const override;
		Vector2f getNodeSize(const IGraphNodeType& nodeType, const BaseGraphNode& node, float curZoom) const override;
		float getIconAlpha(const IGraphNodeType& nodeType, bool dim) const override;
		Vector2f getCommentNodeSize(const BaseGraphNode& node, float curZoom) const;

		String getDebugDisplayValue(uint16_t id) const;
	};
}
