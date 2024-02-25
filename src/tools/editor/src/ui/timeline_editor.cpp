#include "timeline_editor.h"

using namespace Halley;

TimelineEditor::TimelineEditor(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(10, 400), UISizer())
{
	factory.loadUI(*this, "halley/timeline_editor");
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

void TimelineEditor::open(const String& id, std::shared_ptr<Timeline> timeline, ITimelineEditorCallbacks& callbacks)
{
	targetId = id;
	this->callbacks = &callbacks;
	this->timeline = std::move(timeline);

	loadTimeline();

	setActive(true);
}

bool TimelineEditor::isRecording() const
{
	return recording;
}

void TimelineEditor::addChange(const String& targetId, const String& fieldGroupId, const String& fieldId, const String& fieldSubKeyId, const ConfigNode& data)
{
	// TODO
	Logger::logInfo(targetId + " " + fieldGroupId + "::" + fieldId + (fieldSubKeyId.isEmpty() ? "" : "." + fieldSubKeyId) + " = " + data.asString());
}

void TimelineEditor::loadTimeline()
{
	// TODO
}

void TimelineEditor::saveTimeline()
{
	if (callbacks) {
		callbacks->saveTimeline(targetId, *timeline);
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
