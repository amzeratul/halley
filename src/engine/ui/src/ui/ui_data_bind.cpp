#include "halley/ui/ui_data_bind.h"
#include "halley/support/exception.h"
#include "halley/text/halleystring.h"
#include "halley/text/string_converter.h"
#include "ui_widget.h"

using namespace Halley;

UIDataBind::~UIDataBind()
{
}

void UIDataBind::onDataFromWidget(int)
{
}

void UIDataBind::onDataFromWidget(float)
{
}

void UIDataBind::onDataFromWidget(const String&)
{
}

int UIDataBind::getIntData()
{
	throw Exception("Not implemented");
}

float UIDataBind::getFloatData()
{
	throw Exception("Not implemented");
}

String UIDataBind::getStringData()
{
	throw Exception("Not implemented");
}

void UIDataBind::pushData()
{
	Expects(widgetBound != nullptr);
	widgetBound->readFromDataBind();
}

void UIDataBind::setWidget(UIWidget* widget)
{
	Expects((widgetBound == nullptr) ^ (widget == nullptr));
	widgetBound = widget;
}

UIDataBindInt::UIDataBindInt(int initialValue, WriteCallback writeCallback)
	: initialValue(initialValue)
	, writeCallback(writeCallback)
{
}

int UIDataBindInt::getIntData()
{
	return initialValue;
}

float UIDataBindInt::getFloatData()
{
	return float(initialValue);
}

String UIDataBindInt::getStringData()
{
	return toString(initialValue);
}

void UIDataBindInt::onDataFromWidget(int data)
{
	if (writeCallback) {
		writeCallback(data);
	}
}

void UIDataBindInt::onDataFromWidget(float data)
{
	if (writeCallback) {
		writeCallback(lround(data));
	}
}

void UIDataBindInt::onDataFromWidget(const String& data)
{
	if (writeCallback) {
		writeCallback(data.toInteger());
	}
}

UIDataBindFloat::UIDataBindFloat(float initialValue, WriteCallback writeCallback)
	: initialValue(initialValue)
	, writeCallback(writeCallback)
{
}

int UIDataBindFloat::getIntData()
{
	return lround(initialValue);
}

float UIDataBindFloat::getFloatData()
{
	return initialValue;
}

String UIDataBindFloat::getStringData()
{
	return toString(initialValue);
}

void UIDataBindFloat::onDataFromWidget(int data)
{
	if (writeCallback) {
		writeCallback(float(data));
	}
}

void UIDataBindFloat::onDataFromWidget(float data)
{
	if (writeCallback) {
		writeCallback(data);
	}
}

void UIDataBindFloat::onDataFromWidget(const String& data)
{
	if (writeCallback) {
		writeCallback(data.toFloat());
	}
}

UIDataBindString::UIDataBindString(String initialValue, WriteCallback writeCallback)
	: initialValue(std::move(initialValue))
	, writeCallback(writeCallback)
{
}

int UIDataBindString::getIntData()
{
	return initialValue.toInteger();
}

float UIDataBindString::getFloatData()
{
	return initialValue.toFloat();
}

String UIDataBindString::getStringData()
{
	return initialValue;
}

void UIDataBindString::onDataFromWidget(int data)
{
	if (writeCallback) {
		writeCallback(toString(data));
	}
}

void UIDataBindString::onDataFromWidget(float data)
{
	if (writeCallback) {
		writeCallback(toString(data));
	}
}

void UIDataBindString::onDataFromWidget(const String& data)
{
	if (writeCallback) {
		writeCallback(data);
	}
}
