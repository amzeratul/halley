#pragma once
#include "halley/core/graphics/sprite/sprite.h"
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
		ScriptRenderer(Resources& resources, World& world, const ScriptNodeTypeCollection& nodeTypeCollection, float nativeZoom);
		
		void setGraph(const ScriptGraph* graph);
		void setState(const ScriptState* scriptState);
		void draw(Painter& painter, Vector2f basePos, float curZoom);
		
		std::optional<std::pair<uint32_t, Rect4f>> getNodeUnderMouse(Vector2f basePos, float curZoom, std::optional<Vector2f> mousePos) const;
		void setHighlight(std::optional<uint32_t> highlightNode);

	private:
		enum class NodeElementType {
			Input,
			Output,
			Target
		};

		enum class NodeDrawMode {
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
		std::map<String, Sprite> icons;

		std::optional<uint32_t> highlightNode;

		void drawNodeOutputs(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, const ScriptGraph& graph, float curZoom);
		void drawNode(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, float curZoom, NodeDrawMode drawMode);

		Vector2f getNodeSize(float curZoom) const;
		Vector2f getNodeElementPosition(const IScriptNodeType& nodeType, NodeElementType type, Vector2f basePos, const ScriptGraphNode& node, size_t elemIdx, float curZoom) const;
		Colour4f getNodeColour(const IScriptNodeType& nodeType) const;
		const Sprite& getIcon(const IScriptNodeType& nodeType);
	};
}
