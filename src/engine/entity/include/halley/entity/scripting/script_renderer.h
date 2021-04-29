#pragma once
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/maths/circle.h"
#include "halley/maths/vector2.h"

namespace Halley {
	class IScriptNodeType;
	class ScriptNodeTypeCollection;
	class World;
	class ScriptGraphNode;
	class ScriptGraph;
	class ScriptState;

	class ScriptRenderer {
	public:
		enum class NodeElementType : uint8_t {
			Node,
			Input,
			Output,
			Target
		};

		struct NodeUnderMouseInfo {
			uint32_t nodeId;
			NodeElementType elementType;
			uint8_t elementId;
			Rect4f nodeArea;
		};
		
		ScriptRenderer(Resources& resources, World& world, const ScriptNodeTypeCollection& nodeTypeCollection, float nativeZoom);
		
		void setGraph(const ScriptGraph* graph);
		void setState(const ScriptState* scriptState);
		void draw(Painter& painter, Vector2f basePos, float curZoom);
		
		std::optional<NodeUnderMouseInfo> getNodeUnderMouse(Vector2f basePos, float curZoom, std::optional<Vector2f> mousePos) const;
		void setHighlight(std::optional<NodeUnderMouseInfo> highlightNode);

	private:
		enum class NodeDrawMode : uint8_t {
			Normal,
			Highlight,
			Dimmed
		};
		
		Resources& resources;
		World& world;
		const ScriptNodeTypeCollection& nodeTypeCollection;
		float nativeZoom = 1.0f;
		
		const ScriptGraph* graph = nullptr;
		const ScriptState* state = nullptr;

		Sprite nodeBg;
		Sprite pinSprite;
		std::map<String, Sprite> icons;

		std::optional<NodeUnderMouseInfo> highlightNode;

		void drawNodeOutputs(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, const ScriptGraph& graph, float curZoom);
		void drawNode(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, float curZoom, NodeDrawMode drawMode, std::optional<NodeElementType> highlightElement, uint8_t highlightElementId);

		Vector2f getNodeSize(float curZoom) const;
		Circle getNodeElementArea(const IScriptNodeType& nodeType, NodeElementType type, Vector2f basePos, const ScriptGraphNode& node, size_t elemIdx, float curZoom) const;
		Colour4f getNodeColour(const IScriptNodeType& nodeType) const;
		const Sprite& getIcon(const IScriptNodeType& nodeType);
	};
}
