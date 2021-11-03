#include "ui_event.h"

using namespace Halley;

UIEvent::UIEvent()
	: type(UIEventType::Undefined)
{
}

UIEvent::UIEvent(UIEventType type, String sourceId, String data)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["str"] = std::move(data);
}

UIEvent::UIEvent(UIEventType type, String sourceId, bool data)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["bool"] = data;
}

UIEvent::UIEvent(UIEventType type, String sourceId, int data)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["int"] = data;
}

UIEvent::UIEvent(UIEventType type, String sourceId, int data1, int data2)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["int"] = data1;
	configData["int2"] = data2;
}

UIEvent::UIEvent(UIEventType type, String sourceId, KeyCode keyCode, KeyMods keyMods)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["keyCode"] = static_cast<int>(keyCode);
	configData["keyMods"] = static_cast<int>(keyMods);
}

UIEvent::UIEvent(UIEventType type, String sourceId, float data)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["float"] = data;
}

UIEvent::UIEvent(UIEventType type, String sourceId, String data, int intData)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["int"] = intData;
	configData["str"] = std::move(data);
}

UIEvent::UIEvent(UIEventType type, String sourceId, String data, String data2, int intData)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["int"] = intData;
	configData["str"] = std::move(data);
	configData["str2"] = std::move(data2);
}

UIEvent::UIEvent(UIEventType type, String sourceId, Vector2f data)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["vector"] = data;
}

UIEvent::UIEvent(UIEventType type, String sourceId, Rect4f data)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["rect0"] = data.getTopLeft();
	configData["rect1"] = data.getBottomRight();
}

UIEvent::UIEvent(UIEventType type, String sourceId, ConfigNode data)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(std::move(data))
{
}

bool UIEvent::getBoolData() const
{
	return configData["bool"].asBool();
}

int UIEvent::getIntData() const
{
	return configData["int"].asInt();
}

int UIEvent::getIntData2() const
{
	return configData["int2"].asInt();
}

KeyCode UIEvent::getKeyCode() const
{
	return KeyCode(configData["keyCode"].asInt());
}

KeyMods UIEvent::getKeyMods() const
{
	return KeyMods(configData["keyMods"].asInt());
}

float UIEvent::getFloatData() const
{
	return configData["float"].asFloat();
}

Vector2f UIEvent::getVectorData() const
{
	return configData["vector"].asVector2f();
}

Rect4f UIEvent::getRectData() const
{
	return Rect4f(configData["rect0"].asVector2f(), configData["rect1"].asVector2f());
}

const ConfigNode& UIEvent::getConfigData() const
{
	return configData;
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
	return configData["str"].asString();
}

String UIEvent::getStringData() const
{
	return configData["str"].asString();
}

String UIEvent::getStringData2() const
{
	return configData["str2"].asString();
}
