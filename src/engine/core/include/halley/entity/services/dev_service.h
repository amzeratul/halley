#pragma once

#include "halley/scripting/script_renderer.h"
#include "halley/storage/options.h"
#include "halley/entity/service.h"
#include "halley/data_structures/tree_map.h"
#include "halley/entity/entity.h"
#include "halley/maths/polygon.h"
#include "halley/graphics/camera.h"

namespace Halley {
	class UIDebugConsoleCommands;
	class Polygon;
	struct DebugLine;
	struct DebugEllipse;
	struct DebugPolygon;
	struct DebugText;
	struct DebugPoint;
	struct DebugWorldText;

	class DevService : public Service {
	public:
		DevService(bool editorMode = true, bool devMode = true, std::shared_ptr<Options> options = {});

		bool isDevMode() const;
		bool isEditorMode() const;
		
		void setTime(Time time);
		Time getTime() const;

		void setCommands(std::shared_ptr<UIDebugConsoleCommands> commands);
		UIDebugConsoleCommands& getConsoleCommands();

		void setEditorTool(String tool);
		const String& getEditorTool() const;
		
		void setCurrentEntitySubWorld(std::optional<int> subWorld);
		void setSubWorldDisplayOverride(std::optional<int> subWorld);
		std::optional<int> getSubWorldDisplayOverride() const;

		void setSelectedEntities(Vector<EntityId> ids);
		const Vector<EntityId>& getSelectedEntities() const;
		void setEntitiesUnderCursor(const Vector<EntityRef>& entities);
		const Vector<EntityRef>* getEntitiesUnderCursor() const;

		void setSceneBoundaries(Polygon sceneBoundaries, Polygon safeSceneBoundaries);
		void setAltSceneBoundaries(Polygon altSceneBoundaries);
		const Polygon& getSceneBoundaries() const;
		const Polygon& getSafeSceneBoundaries() const;
		const Polygon& getAltSceneBoundaries() const;
		bool hasSceneBoundaries() const;
		bool hasAltSceneBoundaries() const;

		bool getAllowCameraTargets() const;
		void setAllowCameraTargets(bool allowed);

		void setMousePos(std::optional<Vector2f> pos);
		std::optional<Vector2f> getMousePos() const;

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

		void setDevValue(std::string_view name, float value);
		float getDevValue(std::string_view name, float defaultValue) const;

		void setDevFlag(std::string_view name, bool value);
		bool getDevFlag(std::string_view name, bool defaultValue) const;

		const std::optional<String>& getRenderNodeOverride() const;
		void setRenderNodeOverride(std::optional<String> node);

		void setDarkenChunks(bool value);
		bool getDarkenChunks() const;

		void setIsScene(bool isScene);
		bool isScene() const;

	private:
		std::shared_ptr<Options> options;
		bool editorMode = false;
		bool sceneMode = false;
		bool devMode = false;
		Time time = 0;

		std::shared_ptr<UIDebugConsoleCommands> commands;

		String editorTool;
		const Vector<EntityRef>* entitiesUnderCursor = nullptr;

		std::optional<int> curEntitySubWorld;
		std::optional<int> subWorldDisplayOverride;
		Vector<EntityId> selected;

		Polygon sceneBoundaries;
		Polygon safeSceneBoundaries;
		Polygon altSceneBoundaries;
		Prefab* currentEditorScene = nullptr;

		bool allowCameraTargets = true;
		bool darkenChunks = true;

		std::optional<Vector2f> mousePos;

		std::unique_ptr<ScriptRenderer> scriptGraphRenderer;

		std::optional<String> renderNodeOverride;

		Camera devCamera;
		Camera devUICamera;
	};
}

using DevService = Halley::DevService;
