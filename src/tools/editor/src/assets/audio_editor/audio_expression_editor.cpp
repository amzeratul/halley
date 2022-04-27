#include "audio_expression_editor.h"

#include "audio_object_editor.h"
#include "halley/core/properties/audio_properties.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/widgets/ui_list.h"
using namespace Halley;

AudioExpressionEditor::AudioExpressionEditor(UIFactory& factory, AudioExpression& expression, AudioObjectEditor& editor)
	: UIWidget("audio_expression_editor", Vector2f(), UISizer())
	, factory(factory)
	, expression(expression)
	, editor(editor)
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

AudioExpressionTerm& AudioExpressionEditor::getExpressionTerm(size_t idx)
{
	return expression.getTerms()[idx];
}

void AudioExpressionEditor::markModified(size_t idx)
{
	editor.markModified(false);
}

AudioObjectEditor& AudioExpressionEditor::getEditor()
{
	return editor;
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
	const auto& expression = parent.getExpressionTerm(idx);

	if (expression.type == AudioExpressionTermType::Switch) {
		getWidget("switchExpression")->setActive(true);

		getWidgetAs<UIDropdown>("switchId")->setOptions(parent.getEditor().getAudioProperties().getSwitchIds());

		auto updateSwitchValues = [=] (const String& value) {
			const auto& audioProperties = parent.getEditor().getAudioProperties();
			const auto* switchConf = audioProperties.tryGetSwitch(value);
			if (switchConf) {
				getWidgetAs<UIDropdown>("switchValue")->setOptions(switchConf->getValues());
			} else {
				getWidgetAs<UIDropdown>("switchValue")->clear();
			}
		};

		updateSwitchValues(expression.id);

		bindData("switchId", expression.id, [=] (String value)
		{
			updateSwitchValues(value);
			auto& expression = parent.getExpressionTerm(idx);
			expression.id = std::move(value);
			parent.markModified(idx);
		});

		bindData("switchOp", toString(expression.op), [this] (String value)
		{
			auto& expression = parent.getExpressionTerm(idx);
			expression.op = fromString<AudioExpressionTermOp>(value);
			parent.markModified(idx);
		});

		bindData("switchValue", expression.value, [this] (String value)
		{
			auto& expression = parent.getExpressionTerm(idx);
			expression.value = std::move(value);
			parent.markModified(idx);
		});
	} else if (expression.type == AudioExpressionTermType::Variable) {
		// TODO
	}
}
