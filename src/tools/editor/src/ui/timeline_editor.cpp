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
}

void TimelineEditor::open(const String& id, std::shared_ptr<Timeline> timeline, SaveCallback callback)
{
	targetId = id;
	this->callback = std::move(callback);
	this->timeline = std::move(timeline);

	loadTimeline();

	setActive(true);
}

void TimelineEditor::loadTimeline()
{
	// TODO
}

void TimelineEditor::saveTimeline()
{
	if (callback) {
		callback(targetId, *timeline);
	}
}
