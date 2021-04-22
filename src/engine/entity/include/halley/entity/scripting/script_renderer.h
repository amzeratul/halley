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
		ScriptRenderer(Resources& resources, World& world, const ScriptNodeTypeCollection& nodeTypeCollection);
		
		void setGraph(const ScriptGraph* graph);
		void setState(const ScriptState* scriptState);
		void draw(Painter& painter, Vector2f basePos, float curZoom);

	private:
		enum class NodeElementType {
			Input,
			Output,
			Target
		};
		
		Resources& resources;
		World& world;
		const ScriptNodeTypeCollection& nodeTypeCollection;
		
		const ScriptGraph* graph = nullptr;
		const ScriptState* state = nullptr;

		Sprite nodeBg;

		void drawNodeOutputs(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, const ScriptGraph& graph, float curZoom);
		void drawNode(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, float curZoom);

		Vector2f getNodeSize(float curZoom) const;
		Vector2f getNodeElementPosition(const IScriptNodeType& nodeType, NodeElementType type, Vector2f basePos, const ScriptGraphNode& node, size_t elemIdx, float curZoom) const;
		Colour4f getNodeColour(const IScriptNodeType& nodeType) const;
	};
}
