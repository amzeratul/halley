#include "ui_event.h"

using namespace Halley;

UIEvent::UIEvent()
	: type(UIEventType::Undefined)
{
}

UIEvent::UIEvent(UIEventType type, String sourceId, String data)
	: type(type)
	, sourceId(std::move(sourceId))
	, strData(std::move(data))
{
}

UIEvent::UIEvent(UIEventType type, String sourceId, bool data)
	: type(type)
	, sourceId(std::move(sourceId))
	, boolData(data)
{
}

UIEvent::UIEvent(UIEventType type, String sourceId, int data)
	: type(type)
	, intData(data)
	, sourceId(std::move(sourceId))
{
}

UIEvent::UIEvent(UIEventType type, String sourceId, int data1, int data2)
	: type(type)
	, intData(data1)
	, intData2(data2)
	, sourceId(std::move(sourceId))
{
}

UIEvent::UIEvent(UIEventType type, String sourceId, KeyCode keyCode, KeyMods keyMods)
	: type(type)
	, intData(static_cast<int>(keyCode))
	, intData2(static_cast<int>(keyMods))
	, sourceId(std::move(sourceId))
{
}

UIEvent::UIEvent(UIEventType type, String sourceId, float data)
	: type(type)
	, floatData(data)
	, sourceId(std::move(sourceId))
{
}

UIEvent::UIEvent(UIEventType type, String sourceId, String data, int intData)
	: type(type)
	, intData(intData)
	, sourceId(std::move(sourceId))
	, strData(std::move(data))
{
}

UIEvent::UIEvent(UIEventType type, String sourceId, String data, String data2, int intData)
	: type(type)
	, intData(intData)
	, sourceId(std::move(sourceId))
	, strData(std::move(data))
	, strData2(std::move(data2))
{
}

UIEvent::UIEvent(UIEventType type, String sourceId, Vector2f data)
	: type(type)
	, sourceId(std::move(sourceId))
	, vectorData(data)
{
}

UIEvent::UIEvent(UIEventType type, String sourceId, Rect4f data)
	: type(type)
	, sourceId(std::move(sourceId))
	, rectData(data)
{
}

bool UIEvent::getBoolData() const
{
	return boolData;
}

int UIEvent::getIntData() const
{
	return intData;
}

int UIEvent::getIntData2() const
{
	return intData2;
}

KeyCode UIEvent::getKeyCode() const
{
	return KeyCode(intData);
}

KeyMods UIEvent::getKeyMods() const
{
	return KeyMods(intData2);
}

float UIEvent::getFloatData() const
{
	return floatData;
}

Vector2f UIEvent::getVectorData() const
{
	return vectorData;
}

Rect4f UIEvent::getRectData() const
{
	return rectData;
}

UIWidget& UIEvent::getCurWidget() const
{
	return *curWidget;
}

void UIEvent::setCurWidget(UIWidget* widget)
{
	curWidget = widget;
}

UIEventType UIEvent::getType() const
{
	return type;
}

const String& UIEvent::getSourceId() const
{
	return sourceId;
}

String UIEvent::getData() const
{
	return strData;
}

const String& UIEvent::getStringData() const
{
	return strData;
}

const String& UIEvent::getStringData2() const
{
	return strData2;
}
