#include "ui/ui_event.h"
#include "halley/support/exception.h"

using namespace Halley;

UIEvent::UIEvent()
	: type(UIEventType::Undefined)
{
}

UIEvent::UIEvent(UIEventType type, String sourceId, String data)
	: type(type)
	, sourceId(sourceId)
	, data(data)
{
}

UIEventType UIEvent::getType() const
{
	return type;
}

String UIEvent::getSourceId() const
{
	return sourceId;
}

String UIEvent::getData() const
{
	return data;
}

void UIEventHandler::setHandle(UIEventType type, UIEventCallback handler)
{
	handles[type] = handler;
}

void UIEventHandler::setHandle(UIEventType type, String id, UIEventCallback handler)
{
	specificHandles[std::make_pair(type, id)] = handler;
}

bool UIEventHandler::canHandle(const UIEvent& event) const
{
	if (specificHandles.find(std::make_pair(event.getType(), event.getSourceId())) != specificHandles.end()) {
		return true;
	} else {
		return handles.find(event.getType()) != handles.end();
	}
}

void UIEventHandler::queue(const UIEvent& event)
{
	eventQueue.push_back(event);
}

void UIEventHandler::pump()
{
	for (auto& event: eventQueue) {
		handle(event);
	}
	eventQueue.clear();
}

void UIEventHandler::handle(const UIEvent& event) const
{
	auto iter = specificHandles.find(std::make_pair(event.getType(), event.getSourceId()));
	if (iter != specificHandles.end()) {
		iter->second(event);
	} else {
		auto iter2 = handles.find(event.getType());
		if (iter2 != handles.end()) {
			iter2->second(event);
		} else {
			throw Exception("Unable to handle event!");
		}
	}
}
