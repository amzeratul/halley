#include "ui_event_handler.h"
#include "halley/support/exception.h"
#include <utility>

using namespace Halley;

void UIEventHandler::setHandle(UIEventType type, UIEventCallback handler)
{
	handles[type] = std::move(handler);
}

void UIEventHandler::setHandle(UIEventType type, const String& id, UIEventCallback handler)
{
	specificHandles[std::make_pair(type, id)] = std::move(handler);
}

bool UIEventHandler::canHandle(const UIEvent& event) const
{
	if (specificHandles.find(std::make_pair(event.getType(), event.getSourceId())) != specificHandles.end()) {
		return true;
	} else {
		return handles.find(event.getType()) != handles.end();
	}
}

void UIEventHandler::queue(UIEvent event)
{
	eventQueue.emplace_back(std::move(event));
}

void UIEventHandler::pump()
{
	while (!eventQueue.empty()) {
		decltype(eventQueue) events = std::move(eventQueue);
		eventQueue.clear();
		for (auto& event: events) {
			handle(event);
		}
	}
}

void UIEventHandler::setWidget(UIWidget* uiWidget)
{
	widget = uiWidget;
}

void UIEventHandler::handle(UIEvent& event)
{
	auto iter = specificHandles.find(std::make_pair(event.getType(), event.getSourceId()));
	if (iter != specificHandles.end()) {
		event.setCurWidget(widget);
		iter->second(event);
	} else {
		auto iter2 = handles.find(event.getType());
		if (iter2 != handles.end()) {
			event.setCurWidget(widget);
			iter2->second(event);
		} else {
			throw Exception("Unable to handle event!", HalleyExceptions::UI);
		}
	}
}
