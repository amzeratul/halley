#pragma once

namespace Halley {
	class Painter;
	class ScriptGraph;

	class ScriptRenderer {
	public:
		void setGraph(const ScriptGraph& graph);
		void draw(Painter& painter);

	private:
		const ScriptGraph* graph = nullptr;
	};
}