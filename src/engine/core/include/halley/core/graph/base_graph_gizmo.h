#pragma once
#include "base_graph_enums.h"
#include "halley/core/graphics/text/text_renderer.h"
#include "halley/maths/vector2.h"

namespace Halley {
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

		void setUIRoot(UIRoot& root);
		void setEventSink(UIWidget& eventSink);

		void setZoom(float zoom);
		float getZoom() const;
		void setBasePosition(Vector2f pos);

		void onModified();
		void setModifiedCallback(ModifiedCallback callback);

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

		void updateNodeAutoConnection(gsl::span<const GraphNodeId> nodes);
		void pruneConflictingAutoConnections();
		bool finishAutoConnection();
		std::optional<Connection> findAutoConnectionForPin(GraphNodeId srcNodeId, GraphPinId srcPinIdx, Vector2f nodePos, GraphNodePinType srcPinType, gsl::span<const GraphNodeId> excludeIds) const;
	};
}
