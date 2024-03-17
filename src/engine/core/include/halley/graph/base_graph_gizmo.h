#pragma once
#include "base_graph_enums.h"
#include "base_graph_renderer.h"
#include "halley/graphics/text/text_renderer.h"
#include "halley/maths/vector2.h"
#include "halley/data_structures/selection_set.h"
#include "halley/input/input_keys.h"

namespace Halley {
	struct SceneEditorInputState;
	class BaseGraphRenderer;
	class BaseGraph;
	class UIRoot;
	class UIWidget;
	class Resources;
	class IEntityEditorFactory;
	class UIFactory;

	class BaseGraphGizmo {
	public:
		using ModifiedCallback = std::function<void()>;

		BaseGraphGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, Resources& resources, float baseZoom = 1.0f);

		virtual void update(Time time, const SceneEditorInputState& inputState);
		virtual void draw(Painter& painter) const;

		void setUIRoot(UIRoot& root);
		void setEventSink(UIWidget& eventSink);

		void setZoom(float zoom);
		float getZoom() const;
		void setBasePosition(Vector2f pos);

		void onModified();
		void setModifiedCallback(ModifiedCallback callback);

		void onMouseWheel(Vector2f mousePos, int amount, KeyMods keyMods);

		bool isHighlighted() const;
		std::optional<BaseGraphRenderer::NodeUnderMouseInfo> getNodeUnderMouse() const;

		virtual void addNode();
		GraphNodeId addNode(const String& type, Vector2f pos, ConfigNode settings);
		bool destroyNode(GraphNodeId id);
		bool destroyNodes(Vector<GraphNodeId> ids);

	protected:
		struct Dragging {
			Vector<GraphNodeId> nodeIds;
			Vector<Vector2f> startPos;
			std::optional<Vector2f> startMousePos;
			bool sticky = false;
			bool hadChange = false;
		};

		struct Connection {
			GraphNodeId srcNode;
			GraphNodeId dstNode;
			GraphPinId srcPin;
			GraphPinId dstPin;
			GraphNodePinType srcType;
			GraphNodePinType dstType;
			Vector2f srcPos;
			Vector2f dstPos;
			float distance;

			bool operator<(const Connection& other) const;
			bool conflictsWith(const Connection& connection) const;
		};

		UIFactory& factory;
		const IEntityEditorFactory& entityEditorFactory;
		Resources* resources = nullptr;

		std::shared_ptr<BaseGraphRenderer> renderer;
		BaseGraph* baseGraph = nullptr;

		UIRoot* uiRoot = nullptr;
		UIWidget* eventSink = nullptr;

		Vector2f basePos;
		float zoom = 1.0f;
		float baseZoom = 1.0f;

		mutable TextRenderer tooltipLabel;

		ModifiedCallback modifiedCallback;

		Vector<Connection> pendingAutoConnections;

		std::optional<Dragging> dragging;
		SelectionSet<GraphNodeId> selectedNodes;

		static constexpr float gridSize = 16.0f;
		std::optional<BaseGraphRenderer::NodeUnderMouseInfo> nodeUnderMouse;
		std::optional<BaseGraphRenderer::NodeUnderMouseInfo> nodeEditingConnection;
		std::optional<Vector2f> nodeConnectionDst;
		std::optional<Vector2f> lastMousePos;
		bool lastCtrlHeld = false;
		bool lastShiftHeld = false;
		bool autoConnectPin = false;

		void updateNodeAutoConnection(gsl::span<const GraphNodeId> nodes);
		void pruneConflictingAutoConnections();
		bool finishAutoConnection();
		std::optional<Connection> findAutoConnectionForPin(GraphNodeId srcNodeId, GraphPinId srcPinIdx, Vector2f nodePos, GraphNodePinType srcPinType, gsl::span<const GraphNodeId> excludeIds) const;

		void onNodeClicked(Vector2f mousePos, SelectionSetModifier modifier);
		void onNodeDragging(const SceneEditorInputState& inputState);
		void onPinClicked(bool leftClick, bool shiftHeld);
		void onEditingConnection(const SceneEditorInputState& inputState);
		virtual void onNodeAdded(GraphNodeId id);

		SelectionSetModifier getSelectionModifier(const SceneEditorInputState& inputState) const;

		virtual bool canDeleteNode(const BaseGraphNode& node) const;
		virtual bool nodeTypeNeedsSettings(const String& nodeType) const;

		void openNodeUI(std::optional<GraphNodeId> nodeId, std::optional<Vector2f> pos, const String& nodeType);
		virtual void openNodeSettings(std::optional<GraphNodeId> nodeId, std::optional<Vector2f> pos, const String& nodeType);
	};
}
