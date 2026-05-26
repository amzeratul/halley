#include "halley/ui/ui_event.h"

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

UIEvent::UIEvent(UIEventType type, String sourceId, bool data1, bool data2)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["bool"] = data1;
	configData["bool2"] = data2;
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

UIEvent::UIEvent(UIEventType type, String sourceId, int data, KeyMods keyMods)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["int"] = data;
	configData["keyMods"] = static_cast<int>(keyMods);
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

UIEvent::UIEvent(UIEventType type, String sourceId, String data, bool boolData)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["bool"] = boolData;
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

UIEvent::UIEvent(UIEventType type, String sourceId, Vector2f data, int intData, KeyMods keyMods)
	: type(type)
	, sourceId(std::move(sourceId))
	, configData(ConfigNode::MapType())
{
	configData["vector"] = data;
	configData["int"] = intData;
	configData["keyMods"] = static_cast<int>(keyMods);
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
	return getRawData("bool").asBool();
}

bool UIEvent::getBoolData2() const
{
	return getRawData("bool2").asBool();
}

int UIEvent::getIntData() const
{
	return getRawData("int").asInt();
}

int UIEvent::getIntData2() const
{
	return getRawData("int2").asInt();
}

KeyCode UIEvent::getKeyCode() const
{
	return KeyCode(getRawData("keyCode").asInt());
}

KeyMods UIEvent::getKeyMods() const
{
	return KeyMods(getRawData("keyMods").asInt());
}

float UIEvent::getFloatData() const
{
	return getRawData("float").asFloat();
}

Vector2f UIEvent::getVectorData() const
{
	return getRawData("vector").asVector2f();
}

Rect4f UIEvent::getRectData() const
{
	return Rect4f(getRawData("rect0").asVector2f(), getRawData("rect1").asVector2f());
}

String UIEvent::getData() const
{
	return getRawData("str").asString();
}

String UIEvent::getStringData() const
{
	return getRawData("str").asString();
}

String UIEvent::getStringData2() const
{
	return getRawData("str2").asString();
}

const ConfigNode& UIEvent::getRawData(const String& key) const
{
	if (!configData.hasKey(key)) {
		throw Exception("UIEvent with type " + toString(static_cast<int>(type)) + " has no key \"" + key + "\"", HalleyExceptions::UI);
	}
	return configData[key];
}

const ConfigNode& UIEvent::getConfigData() const
{
	return configData;
}

UIWidget& UIEvent::getCurWidget() const
{
	return *curWidget;
}

UIEventDirection UIEvent::getDirection() const
{
	return direction;
}

void UIEvent::setCurWidget(UIWidget* widget)
{
	curWidget = widget;
}

void UIEvent::setDirection(UIEventDirection direction)
{
	this->direction = direction;
}

UIEventType UIEvent::getType() const
{
	return type;
}

const String& UIEvent::getSourceId() const
{
	return sourceId;
}

