#pragma once
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/maths/vector2.h"

namespace Halley {
	class World;
	class ScriptGraphNode;
	class ScriptGraph;

	class ScriptRenderer {
	public:
		explicit ScriptRenderer(Resources& resources, World& world);
		
		void setGraph(const ScriptGraph& graph);
		void draw(Painter& painter, Vector2f basePos, float curZoom);

	private:
		enum class NodeElementType {
			Input,
			Output,
			Target
		};
		
		Resources& resources;
		World& world;
		const ScriptGraph* graph = nullptr;

		Sprite nodeBg;

		void drawNodeOutputs(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, const ScriptGraph& graph, float curZoom);
		void drawNode(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, float curZoom);

		Vector2f getNodeSize(float curZoom) const;
		Vector2f getNodeElementPosition(NodeElementType type, Vector2f basePos, const ScriptGraphNode& node, size_t elemIdx, float curZoom);
	};
}
