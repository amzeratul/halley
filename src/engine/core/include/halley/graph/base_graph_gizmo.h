#pragma once
#include "base_graph_enums.h"
#include "base_graph_renderer.h"
#include "halley/graphics/text/text_renderer.h"
#include "halley/maths/vector2.h"
#include "halley/data_structures/selection_set.h"
#include "halley/editor_extensions/choose_asset_window.h"
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
		virtual ~BaseGraphGizmo();

		virtual void update(Time time, const SceneEditorInputState& inputState);
		virtual void draw(Painter& painter) const;

		void setBaseGraph(BaseGraph* graph);

		void setUIRoot(UIRoot& root);
		void setEventSink(UIWidget& eventSink);

		void setZoom(float zoom);
		float getZoom() const;
		void setBasePosition(Vector2f pos);
		void setAutoConnectPins(bool autoConnect);

		void onModified();
		void setModifiedCallback(ModifiedCallback callback);

		void onMouseWheel(Vector2f mousePos, int amount, KeyMods keyMods);

		bool isHighlighted() const;
		std::optional<BaseGraphRenderer::NodeUnderMouseInfo> getNodeUnderMouse() const;

		void addNode();
		GraphNodeId addNode(const String& type, Vector2f pos, ConfigNode settings);
		bool destroyNode(GraphNodeId id);
		bool destroyNodes(Vector<GraphNodeId> ids);
		bool isValidPaste(const ConfigNode& node) const;
		bool deleteSelection();

		BaseGraphNode& getNode(GraphNodeId id);

		[[nodiscard]] ConfigNode copySelection() const;
		[[nodiscard]] ConfigNode cutSelection();
		void paste(const ConfigNode& node);
		void copySelectionToClipboard(const std::shared_ptr<IClipboard>& clipboard) const;
		void cutSelectionToClipboard(const std::shared_ptr<IClipboard>& clipboard);
		void pasteFromClipboard(const std::shared_ptr<IClipboard>& clipboard);

		ExecutionQueue& getExecutionQueue();

	protected:
		UIFactory& factory;
		const IEntityEditorFactory& entityEditorFactory;
		Resources* resources = nullptr;
		std::shared_ptr<BaseGraphRenderer> renderer;

		UIRoot* uiRoot = nullptr;
		UIWidget* eventSink = nullptr;

		virtual void refreshNodes() const;
		virtual bool canDeleteNode(const BaseGraphNode& node) const;
		virtual bool nodeTypeNeedsSettings(const String& nodeType) const;
		virtual void openNodeSettings(std::optional<GraphNodeId> nodeId, std::optional<Vector2f> pos, const String& nodeType);
		virtual std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const BaseGraphRenderer::NodeUnderMouseInfo& nodeInfo) const;
		virtual std::shared_ptr<UIWidget> makeChooseNodeTypeWindow(Vector2f windowSize, UIFactory& factory, Resources& resources, ChooseAssetWindow::Callback callback);
		virtual std::unique_ptr<BaseGraphNode> makeNode(const ConfigNode& node) = 0;
		virtual std::shared_ptr<BaseGraphRenderer> makeRenderer(Resources& resources, float baseZoom) = 0;

	private:
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

		BaseGraph* baseGraph = nullptr;

		Vector<Connection> pendingAutoConnections;
		std::optional<Dragging> dragging;

		static constexpr float gridSize = 16.0f;
		std::optional<BaseGraphRenderer::NodeUnderMouseInfo> nodeUnderMouse;
		std::optional<BaseGraphRenderer::NodeUnderMouseInfo> nodeEditingConnection;
		std::optional<Vector2f> nodeConnectionDst;
		std::optional<Vector2f> lastMousePos;
		SelectionSet<GraphNodeId> selectedNodes;
		bool lastCtrlHeld = false;
		bool lastShiftHeld = false;
		bool autoConnectPin = false;

		Vector2f basePos;
		float zoom = 1.0f;
		float baseZoom = 1.0f;
		mutable TextRenderer tooltipLabel;

		ModifiedCallback modifiedCallback;
		ExecutionQueue pendingUITasks;

		void updateNodeAutoConnection(gsl::span<const GraphNodeId> nodes);
		void pruneConflictingAutoConnections();
		bool finishAutoConnection();
		std::optional<Connection> findAutoConnectionForPin(GraphNodeId srcNodeId, GraphPinId srcPinIdx, Vector2f nodePos, GraphNodePinType srcPinType, gsl::span<const GraphNodeId> excludeIds) const;

		void onNodeClicked(Vector2f mousePos, SelectionSetModifier modifier);
		void onNodeDragging(const SceneEditorInputState& inputState);
		void onPinClicked(bool leftClick, bool shiftHeld);
		void onEditingConnection(const SceneEditorInputState& inputState);
		void openNodeUI(std::optional<GraphNodeId> nodeId, std::optional<Vector2f> pos, const String& nodeType);

		void drawToolTip(Painter& painter, const BaseGraphNode& node, const BaseGraphRenderer::NodeUnderMouseInfo& nodeInfo) const;
		void drawToolTip(Painter& painter, const String& text, const Vector<ColourOverride>& colours, Vector2f pos) const;
		void drawWheelGuides(Painter& painter) const;

		SelectionSetModifier getSelectionModifier(const SceneEditorInputState& inputState) const;
	};
}
