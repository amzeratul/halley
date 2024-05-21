#pragma once

#include "halley/scripting/script_renderer.h"
#include "halley/entity/service.h"
#include "halley/data_structures/tree_map.h"
#include "halley/maths/polygon.h"

namespace Halley {
	class Polygon;
	struct DebugLine;
	struct DebugEllipse;
	struct DebugPolygon;
	struct DebugText;
	struct DebugPoint;
	struct DebugWorldText;

	class DebugDrawService : public Service {
	public:
		DebugDrawService() = default;

		const Vector<DebugLine>& getDebugLines();
		const Vector<DebugPoint>& getDebugPoints();
		const Vector<DebugPolygon>& getDebugPolygons();
		const Vector<DebugEllipse>& getDebugEllipses();
		const Vector<DebugWorldText>& getDebugWorldTexts();
		const TreeMap<String, DebugText>& getDebugTexts();

		void addDebugLine(Vector<Vector2f> line, Colour4f colour, float thickness = 1.0f, bool loop = false);
		void addDebugArrow(Vector2f from, Vector2f to, Colour4f colour, float headSize = 10.0f, float thickness = 1.0f, float sideShift = 0.0f);
		void addDebugPoint(Vector2f point, Colour4f colour, float radius = 1.0f);
		void addDebugPolygon(Polygon polygon, Colour4f colour);
		void addDebugEllipse(Vector2f point, Vector2f radius, Colour4f colour, float thickness = 1.0f);
		void addDebugText(std::string_view key, String value);
		void addDebugText(String value, Vector2f position);

		void initScriptGraphRenderer(Resources& resources, const ScriptNodeTypeCollection& scriptNodeTypeCollection, float nativeZoom);
		void addScriptRenderer(Vector2f pos, std::shared_ptr<ScriptState> state);
		void drawScripts(Painter& painter);

	private:
		std::unique_ptr<ScriptRenderer> scriptGraphRenderer;
	};
}

using DebugDrawService = Halley::DebugDrawService;
