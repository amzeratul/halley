#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIFactory;
	class IScriptNodeType;
	class IEntityEditorFactory;
	class ScriptingBaseGizmo;

	class ScriptingNodeEditor : public UIWidget {
	public:
		ScriptingNodeEditor(ScriptingBaseGizmo& gizmo, UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, UIWidget* eventSink, std::optional<uint32_t> nodeId, const IGraphNodeType& nodeType, std::optional<Vector2f> pos);

		void onMakeUI() override;
		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;

	protected:
		bool onKeyPress(KeyboardKeyPress key) override;
	
	private:
		ScriptingBaseGizmo& gizmo;
		UIFactory& factory;
		const IEntityEditorFactory& entityEditorFactory;
		UIWidget* eventSink = nullptr;
		std::optional<uint32_t> nodeId;
		const IGraphNodeType& nodeType;
		ConfigNode curSettings;

		void applyChanges();
		void cancelChanges();
		void deleteNode();
		void makeFields(const std::shared_ptr<UIWidget>& fieldsRoot);
	};
}
