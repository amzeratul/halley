#include "audio_sequence_editor.h"

#include "audio_fade_editor.h"
#include "audio_object_editor.h"
using namespace Halley;

AudioSequenceEditor::AudioSequenceEditor(UIFactory& factory, AudioObjectEditor& editor, AudioSubObjectSequence& sequence)
	: UIWidget("audio_sequence_editor", Vector2f(), UISizer())
	, factory(factory)
	, editor(editor)
	, sequence(sequence)
{
	factory.loadUI(*this, "halley/audio_editor/audio_sequence_editor");
}

void AudioSequenceEditor::onMakeUI()
{
	getWidget("fadeContainer")->add(std::make_shared<AudioFadeEditor>(factory, sequence.getCrossFade(), [=] ()
	{
		editor.markModified(false);
	}));

	const auto layerList = getWidgetAs<UIList>("segments");
	for (size_t i = 0; i < sequence.getSegments().size(); ++i) {
		layerList->addItem(toString(i), std::make_shared<AudioSequenceEditorSegment>(factory, *this, i), 1);
	}

	bindData("sequenceType", toString(sequence.getSequenceType()), [=] (String value)
	{
		sequence.getSequenceType() = fromString<AudioSequenceType>(value);
		editor.markModified(false);
	});

	setHandle(UIEventType::ListItemsSwapped, [=] (const UIEvent& event)
	{
		std::swap(sequence.getSegments()[event.getIntData()], sequence.getSegments()[event.getIntData2()]);
		editor.markModified(true);
		refreshIds();
	});
}

void AudioSequenceEditor::markModified(size_t idx)
{
	editor.markModified(false);
}

AudioObjectEditor& AudioSequenceEditor::getEditor()
{
	return editor;
}

AudioSubObjectSequence::Segment& AudioSequenceEditor::getSegment(size_t idx)
{
	return sequence.getSegment(idx);
}

void AudioSequenceEditor::refreshIds()
{
	const auto layerList = getWidgetAs<UIList>("layers");
	const auto n = layerList->getCount();
	for (size_t i = 0; i < n; ++i) {
		const auto layerEditor = layerList->getItem(static_cast<int>(i))->getWidgetAs<AudioSequenceEditorSegment>("audio_sequence_editor_segment");
		layerEditor->setIdx(i);
	}
}

AudioSequenceEditorSegment::AudioSequenceEditorSegment(UIFactory& factory, AudioSequenceEditor& sequenceEditor, size_t idx)
	: UIWidget("audio_sequence_editor_segment", {}, UISizer())
	, factory(factory)
	, sequenceEditor(sequenceEditor)
	, idx(idx)
{
	factory.loadUI(*this, "halley/audio_editor/audio_sequence_editor_segment");
}

void AudioSequenceEditorSegment::onMakeUI()
{
	const auto& segment = sequenceEditor.getSegment(idx);
	const auto& name = segment.object->getName();
	getWidgetAs<UILabel>("segmentName")->setText(LocalisedString::fromUserString(name));

	bindData("endSample", segment.endSample, [=] (int value)
	{
		sequenceEditor.getSegment(idx).endSample = value;
		sequenceEditor.markModified(idx);
	});
}

void AudioSequenceEditorSegment::setIdx(size_t idx)
{
	this->idx = idx;
}
