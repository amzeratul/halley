#include "timeline_editor.h"

using namespace Halley;

TimelineEditor::TimelineEditor(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(10, 400), UISizer())
{
	factory.loadUI(*this, "halley/timeline_editor");
}

TimelineEditor::~TimelineEditor()
{
}

void TimelineEditor::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "close", [=] (const UIEvent& event)
	{
		saveTimeline();
		setActive(false);
	});

	setHandle(UIEventType::ButtonClicked, "record", [=] (const UIEvent& event)
	{
		toggleRecording();
	});
}

void TimelineEditor::open(const String& id, std::unique_ptr<Timeline> timeline, ITimelineEditorCallbacks& callbacks)
{
	timelineOwnerId = id;
	this->callbacks = &callbacks;
	this->timeline = std::move(timeline);

	populateTimeline();

	setActive(true);
}

bool TimelineEditor::isRecording() const
{
	return recording;
}

void TimelineEditor::addChange(const String& targetId, const String& fieldGroupId, const String& fieldId, const String& fieldSubKeyId, const ConfigNode& data)
{
	const auto key = TimelineSequence::Key(fieldGroupId, fieldId, fieldSubKeyId);
	auto& seq = timeline->getSequence(targetId, key);
	seq.addKeyFrame(curFrame, ConfigNode(data));

	saveTimeline();
	populateTimeline();
}

void TimelineEditor::populateTimeline()
{
	// TODO
}

void TimelineEditor::saveTimeline()
{
	if (callbacks) {
		callbacks->saveTimeline(timelineOwnerId, *timeline);
	}
}

void TimelineEditor::toggleRecording()
{
	recording = !recording;
	if (recording) {
		getWidgetAs<UIButton>("record")->setLabel(LocalisedString::fromHardcodedString("Stop"));
		callbacks->onStartTimelineRecording();
	} else {
		callbacks->onStopTimelineRecording();
		getWidgetAs<UIButton>("record")->setLabel(LocalisedString::fromHardcodedString("Record"));
	}
}
