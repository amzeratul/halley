#pragma once

#include "halley/ui/ui_widget.h"
#include "src/scene/gizmos/scripting/scripting_base_gizmo.h"

namespace Halley {
	class ScriptGraphEditor;

	class ScriptGizmoUI : public UIWidget {
	public:
		ScriptGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, std::shared_ptr<InputKeyboard> keyboard, std::shared_ptr<IClipboard> clipboard, ScriptGraphEditor& graphEditor);

		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;

		void load(ScriptGraph& graph);
		void setState(ScriptState* state);

		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

		void setZoom(float zoom);
		void setAutoConnectPins(bool autoConnect);
		bool isHighlighted() const;
		
		std::shared_ptr<UIWidget> makeUI();

		std::optional<BaseGraphRenderer::NodeUnderMouseInfo> getNodeUnderMouse() const;
		void setCurNodeDevConData(const String& str);
		void setDebugDisplayData(HashMap<int, String> values);

		void updateNodes();

		void setEntityTargets(Vector<String> entityTargets);

	protected:
        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
		void releaseMouse(Vector2f mousePos, int button) override;
        void onMouseOver(Vector2f mousePos) override;
		void onMouseLeft(Vector2f mousePos) override;
		bool ignoreClip() const override;
		bool onKeyPress(KeyboardKeyPress key) override;
		bool canReceiveFocus() const override;

	private:
		UIFactory& factory;
		Resources& resources;
		std::shared_ptr<InputKeyboard> keyboard;
		std::shared_ptr<IClipboard> clipboard;
		ScriptingBaseGizmo gizmo;
		ScriptGraphEditor& graphEditor;

		SceneEditorInputState inputState;
		std::optional<Vector2f> dragStart;

		void onModified();
		void updateSelectionBox();
		void onDoubleClick(GraphNodeId nodeId);
	};
}
