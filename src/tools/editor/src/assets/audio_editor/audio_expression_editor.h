#pragma once
#include "halley/audio/audio_expression.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIFactory;

	class AudioExpressionEditor : public UIWidget {
	public:
		AudioExpressionEditor(UIFactory& factory, AudioExpression& expression);

		void onMakeUI() override;

	private:
		UIFactory& factory;
		AudioExpression& expression;
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
