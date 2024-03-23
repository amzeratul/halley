#pragma once

namespace Halley {
	class ScriptGraphVariableInspector;

	class IGraphEditor {
	public:
		virtual ~IGraphEditor() = default;

		virtual void setModified(bool modified) = 0;
		virtual void onModified() = 0;
		virtual void undo() = 0;
		virtual void redo() = 0;
	};
}
