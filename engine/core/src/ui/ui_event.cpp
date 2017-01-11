#include "ui/ui_event.h"

using namespace Halley;

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

bool UIEventHandler::handle(const UIEvent& event) const
{
	{
		auto key = std::make_pair(event.getType(), event.getSourceId());
		auto iter = specificHandles.find(key);
		if (iter != specificHandles.end()) {
			iter->second(event);
			return true;
		}
	}

	{
		auto key = event.getType();
		auto iter = handles.find(key);
		if (iter != handles.end()) {
			iter->second(event);
			return true;
		}
	}

	return false;
}
