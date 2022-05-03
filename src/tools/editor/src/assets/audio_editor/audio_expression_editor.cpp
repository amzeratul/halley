#include "audio_expression_editor.h"

#include "audio_object_editor.h"
#include "halley/core/properties/audio_properties.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/widgets/ui_list.h"
#include "src/assets/curve_editor.h"
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
	loadUI();

	setHandle(UIEventType::ButtonClicked, "add", [=] (const UIEvent& event)
	{
		addTerm();
	});

	setHandle(UIEventType::ListItemsSwapped, "expressions", [=] (const UIEvent& event)
	{
		refreshIds();
		editor.markModified(false);
	});
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

void AudioExpressionEditor::deleteTerm(size_t idx)
{
	Concurrent::execute(Executors::getMainUpdateThread(), [=] () {
		getWidgetAs<UIList>("expressions")->removeItem(expressionEditors[idx]->getId());
		expression.getTerms().erase(expression.getTerms().begin() + idx);
		expressionEditors.erase(expressionEditors.begin() + idx);
		refreshIds();

		editor.markModified(false);
	});
}

void AudioExpressionEditor::loadUI()
{
	auto exprList = getWidgetAs<UIList>("expressions");
	exprList->clear();
	expressionEditors.clear();
	size_t idx = 0;
	for (auto& e: expression.getTerms()) {
		expressionEditors.push_back(std::make_shared<AudioExpressionEditorExpression>(factory, *this, idx));
		exprList->addItem(expressionEditors.back()->getId(), expressionEditors.back(), 1);
		++idx;
	}
}

void AudioExpressionEditor::refreshIds()
{
	for (size_t i = 0; i < expressionEditors.size(); ++i) {
		expressionEditors[i]->updateIdx(i);
	}
}

void AudioExpressionEditor::addTerm()
{
	getRoot()->addChild(std::make_shared<ChooseAudioExpressionAction>(factory, [=] (std::optional<String> result)
	{
		if (result) {
			addTerm(fromString<AudioExpressionTermType>(*result));
		}
	}));
}

void AudioExpressionEditor::addTerm(AudioExpressionTermType type)
{
	const auto idx = expression.getTerms().size();
	expression.getTerms().push_back(AudioExpressionTerm(type));

	expressionEditors.push_back(std::make_shared<AudioExpressionEditorExpression>(factory, *this, idx));
	getWidgetAs<UIList>("expressions")->addItem(expressionEditors.back()->getId(), expressionEditors.back(), 1);

	editor.markModified(false);
}

AudioExpressionEditorExpression::AudioExpressionEditorExpression(UIFactory& factory, AudioExpressionEditor& parent, size_t idx)
	: UIWidget("", Vector2f(), UISizer())
	, factory(factory)
	, parent(parent)
	, idx(idx)
{
	static size_t uniqueIdx = 0;
	setId("audio_expression_editor_expression_" + toString(uniqueIdx++));

	factory.loadUI(*this, "halley/audio_editor/audio_expression_editor_expression");
}

void AudioExpressionEditorExpression::onMakeUI()
{
	const auto& expression = parent.getExpressionTerm(idx);
	const auto& audioProperties = parent.getEditor().getAudioProperties();

	if (expression.type == AudioExpressionTermType::Switch) {
		getWidget("switchExpression")->setActive(true);

		getWidgetAs<UIDropdown>("switchId")->setOptions(audioProperties.getSwitchIds());

		auto updateSwitchValues = [=] (const String& value) {
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
		getWidget("variableExpression")->setActive(true);

		getWidgetAs<UIDropdown>("variableId")->setOptions(audioProperties.getVariableIds());

		bindData("variableId", expression.id, [=] (String value)
		{
			auto& expr = parent.getExpressionTerm(idx);
			expr.id = std::move(value);
			parent.markModified(idx);
		});

		auto curveEditor = getWidgetAs<CurveEditor>("variableCurve");
		auto variableConfig = audioProperties.tryGetVariable(expression.id);
		if (variableConfig) {
			curveEditor->setHorizontalRange(variableConfig->getRange());
		} else {
			curveEditor->setHorizontalRange(Range<float>(0, 1));
		}
		curveEditor->setPoints(expression.points);
		curveEditor->setChangeCallback([=] (const Vector<Vector2f>& points)
		{
			auto& expr = parent.getExpressionTerm(idx);
			expr.points = points;
			parent.markModified(idx);
		});
	}

	setHandle(UIEventType::ButtonClicked, "delete", [=] (const UIEvent& event)
	{
		parent.deleteTerm(idx);
	});
}

void AudioExpressionEditorExpression::updateIdx(size_t idx)
{
	this->idx = idx;
}

ChooseAudioExpressionAction::ChooseAudioExpressionAction(UIFactory& factory, Callback callback)
	: ChooseAssetWindow(Vector2f(), factory, std::move(callback), {})
{
	Vector<String> ids;
	Vector<String> names;
	for (auto id: EnumNames<AudioExpressionTermType>()()) {
		ids.push_back(id);
		names.push_back(id);
	}
	setTitle(LocalisedString::fromHardcodedString("Add Expression"));
	setAssetIds(ids, names, "variable");
}

void ChooseAudioExpressionAction::sortItems(Vector<std::pair<String, String>>& values)
{
}
