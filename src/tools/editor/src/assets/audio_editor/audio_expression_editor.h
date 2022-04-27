#pragma once
#include "halley/audio/audio_expression.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class AudioObjectEditor;
	class UIFactory;

	class AudioExpressionEditor : public UIWidget {
	public:
		AudioExpressionEditor(UIFactory& factory, AudioExpression& expression, AudioObjectEditor& editor);

		void onMakeUI() override;

		AudioExpressionTerm& getExpressionTerm(size_t idx);
		void markModified(size_t idx);
		AudioObjectEditor& getEditor();

	private:
		UIFactory& factory;
		AudioExpression& expression;
		AudioObjectEditor& editor;
	};

	class AudioExpressionEditorExpression : public UIWidget {
	public:
		AudioExpressionEditorExpression(UIFactory& factory, AudioExpressionEditor& parent, size_t idx);

		void onMakeUI() override;

	private:
		UIFactory& factory;
		AudioExpressionEditor& parent;
		size_t idx;
	};
}
