#include "audio_expression_editor.h"

#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_list.h"
using namespace Halley;

AudioExpressionEditor::AudioExpressionEditor(UIFactory& factory, AudioExpression& expression)
	: UIWidget("audio_expression_editor", Vector2f(), UISizer())
	, factory(factory)
	, expression(expression)
{
	factory.loadUI(*this, "halley/audio_editor/audio_expression_editor");
}

void AudioExpressionEditor::onMakeUI()
{
	auto exprList = getWidgetAs<UIList>("expressions");
	size_t idx = 0;
	for (auto& e: expression.getTerms()) {
		exprList->addItem(toString(idx), std::make_shared<AudioExpressionEditorExpression>(factory, *this, idx));
		++idx;
	}
}

AudioExpressionEditorExpression::AudioExpressionEditorExpression(UIFactory& factory, AudioExpressionEditor& parent, size_t idx)
	: UIWidget("audio_expression_editor_expression", Vector2f(), UISizer())
	, factory(factory)
	, parent(parent)
	, idx(idx)
{
	factory.loadUI(*this, "halley/audio_editor/audio_expression_editor_expression");
}

void AudioExpressionEditorExpression::onMakeUI()
{

}
