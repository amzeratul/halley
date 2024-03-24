#pragma once

#include "graph_editor.h"
#include "halley/ui/ui_widget.h"
#include "src/scene/gizmos/graph/base_graph_gizmo.h"

namespace Halley {
	class IGraphEditor;
	class ScriptGraphEditor;

	class GraphGizmoUI : public UIWidget {
	public:
		GraphGizmoUI(std::shared_ptr<InputKeyboard> keyboard, std::shared_ptr<IClipboard> clipboard, IGraphEditor& graphEditor, std::unique_ptr<BaseGraphGizmo> gizmo);

		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;

		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

		void setZoom(float zoom);
		void setAutoConnectPins(bool autoConnect);
		bool isHighlighted() const;
		
		std::shared_ptr<UIWidget> makeUI();

		std::optional<BaseGraphRenderer::NodeUnderMouseInfo> getNodeUnderMouse() const;

		virtual void load(BaseGraph& graph) = 0;

	protected:
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
		void releaseMouse(Vector2f mousePos, int button) override;
        void onMouseOver(Vector2f mousePos) override;
		void onMouseLeft(Vector2f mousePos) override;
		bool ignoreClip() const override;
		bool onKeyPress(KeyboardKeyPress key) override;
		bool canReceiveFocus() const override;

		std::unique_ptr<BaseGraphGizmo> gizmo;

	private:
		std::shared_ptr<InputKeyboard> keyboard;
		std::shared_ptr<IClipboard> clipboard;
		IGraphEditor& graphEditor;

		SceneEditorInputState inputState;
		std::optional<Vector2f> dragStart;

		void onModified();
		void updateSelectionBox();
	};
}
