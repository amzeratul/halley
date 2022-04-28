#pragma once
#include "halley/audio/audio_expression.h"
#include "halley/editor_extensions/choose_asset_window.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class AudioObjectEditor;
	class UIFactory;

	class AudioExpressionEditorExpression;

	class AudioExpressionEditor : public UIWidget {
	public:
		AudioExpressionEditor(UIFactory& factory, AudioExpression& expression, AudioObjectEditor& editor);

		void onMakeUI() override;

		AudioExpressionTerm& getExpressionTerm(size_t idx);
		void markModified(size_t idx);
		AudioObjectEditor& getEditor();
		void deleteTerm(size_t idx);

	private:
		UIFactory& factory;
		AudioExpression& expression;
		Vector<std::shared_ptr<AudioExpressionEditorExpression>> expressionEditors;
		AudioObjectEditor& editor;

		void loadUI();
		void refreshIds();
		void addTerm();
		void addTerm(AudioExpressionTermType type);
	};

	class AudioExpressionEditorExpression : public UIWidget {
	public:
		AudioExpressionEditorExpression(UIFactory& factory, AudioExpressionEditor& parent, size_t idx);

		void onMakeUI() override;
		void updateIdx(size_t idx);

	private:
		UIFactory& factory;
		AudioExpressionEditor& parent;
		size_t idx;
	};

	class ChooseAudioExpressionAction : public ChooseAssetWindow {
	public:
        ChooseAudioExpressionAction(UIFactory& factory, Callback callback);

	private:
		void sortItems(Vector<std::pair<String, String>>& items) override;
	};
}
