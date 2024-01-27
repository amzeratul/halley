#include "timeline_editor.h"

using namespace Halley;

TimelineEditor::TimelineEditor(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(10, 200), UISizer())
{
	factory.loadUI(*this, "halley/timeline_editor");
}

void TimelineEditor::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "close", [=] (const UIEvent& event)
	{
		setActive(false);
	});
}
