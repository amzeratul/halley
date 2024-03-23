#pragma once
#include "script_node_type.h"
#include "script_node_enums.h"
#include "halley/graph/base_graph_renderer.h"
#include "halley/graphics/sprite/sprite.h"
#include "halley/maths/circle.h"
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
		ScriptRenderer(Resources& resources, const World* world, const ScriptNodeTypeCollection& nodeTypeCollection, float nativeZoom);
		
		void setState(const ScriptState* scriptState);
		void setDebugDisplayData(HashMap<int, String> values) override;

		static Colour4f getScriptNodeColour(const IScriptNodeType& nodeType);

	protected:
		bool isDimmed(GraphNodePinType type) const override;
		GraphPinSide getSide(GraphNodePinType pinType) const override;
		Colour4f getPinColour(GraphNodePinType pinType) const override;
		Colour4f getBaseNodeColour(const IGraphNodeType& type) const override;
		const IGraphNodeType* tryGetNodeType(const String& typeId) const override;

	private:		
		const World* world = nullptr;
		const ScriptNodeTypeCollection& nodeTypeCollection;
		
		const ScriptState* state = nullptr;

		Sprite nodeBg;
		Sprite nodeBgOutline;
		Sprite pinSprite;
		Sprite destructorBg;
		Sprite destructorIcon;
		TextRenderer labelText;
		HashMap<int, String> debugDisplayValues;

		void drawNodeBackground(Painter& painter, Vector2f basePos, const BaseGraphNode& node, float curZoom, float posScale, NodeDrawMode drawMode) override;
		void drawNode(Painter& painter, Vector2f basePos, const BaseGraphNode& node, float curZoom, float posScale, NodeDrawMode drawMode, std::optional<GraphNodePinType> highlightElement, GraphPinId highlightElementId) override;

		NodeDrawMode getNodeDrawMode(GraphNodeId nodeId) const override;
		Vector2f getNodeSize(const IGraphNodeType& nodeType, const BaseGraphNode& node, float curZoom) const override;
		Vector2f getCommentNodeSize(const BaseGraphNode& node, float curZoom) const;

		String getDebugDisplayValue(uint16_t id) const;
	};
}
