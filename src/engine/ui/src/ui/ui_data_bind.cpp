#include "halley/ui/ui_data_bind.h"
#include "halley/support/exception.h"
#include "halley/text/halleystring.h"
#include "halley/text/string_converter.h"
#include "ui_widget.h"

using namespace Halley;

UIDataBind::~UIDataBind()
{
}

void UIDataBind::onDataFromWidget(bool data)
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

bool UIDataBind::canWriteData() const
{
	return acceptingData;
}

bool UIDataBind::getBoolData()
{
	return getIntData() != 0;
}

int UIDataBind::getIntData()
{
	throw Exception("Not implemented", HalleyExceptions::UI);
}

float UIDataBind::getFloatData()
{
	throw Exception("Not implemented", HalleyExceptions::UI);
}

String UIDataBind::getStringData()
{
	throw Exception("Not implemented", HalleyExceptions::UI);
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

UIDataBind::Format UIDataBindInt::getFormat() const
{
	return Format::Int;
}

void UIDataBindInt::onDataFromWidget(bool data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(data ? 1 : 0);
	}
}

void UIDataBindFloat::onDataFromWidget(bool data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(data ? 1.0f : 0.0f);
	}
}

void UIDataBind::setAcceptingDataFromWidget(bool accepting)
{
	acceptingData = accepting;
}

UIDataBindBool::UIDataBindBool(bool initialValue, WriteCallback writeCallback)
	: initialValue(initialValue)
	, writeCallback(writeCallback)
{
}

bool UIDataBindBool::getBoolData()
{
	return initialValue;
}

int UIDataBindBool::getIntData()
{
	return initialValue ? 1 : 0;
}

float UIDataBindBool::getFloatData()
{
	return initialValue ? 1.0f : 0.0f;
}

String UIDataBindBool::getStringData()
{
	return initialValue ? "true" : "false";
}

UIDataBind::Format UIDataBindBool::getFormat() const
{
	return Format::Bool;
}

void UIDataBindBool::onDataFromWidget(bool data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(data);
	}
}

void UIDataBindBool::onDataFromWidget(int data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(data != 0);
	}
}

void UIDataBindBool::onDataFromWidget(float data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(data != 0);
	}
}

void UIDataBindBool::onDataFromWidget(const String& data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(data == "true");
	}
}

void UIDataBindInt::onDataFromWidget(int data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(data);
	}
}

void UIDataBindInt::onDataFromWidget(float data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(lround(data));
	}
}

void UIDataBindInt::onDataFromWidget(const String& data)
{
	if (canWriteData() && writeCallback && data.isInteger()) {
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

UIDataBind::Format UIDataBindFloat::getFormat() const
{
	return Format::Float;
}

void UIDataBindFloat::onDataFromWidget(int data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(float(data));
	}
}

void UIDataBindFloat::onDataFromWidget(float data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(data);
	}
}

void UIDataBindFloat::onDataFromWidget(const String& data)
{
	if (canWriteData() && writeCallback && data.isNumber()) {
		writeCallback(data.toFloat());
	}
}

UIDataBindString::UIDataBindString(String initialValue, WriteCallback writeCallback)
	: initialValue(std::move(initialValue))
	, writeCallback(writeCallback)
{
}

bool UIDataBindString::getBoolData()
{
	return initialValue == "true";
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

UIDataBind::Format UIDataBindString::getFormat() const
{
	return Format::String;
}

void UIDataBindString::onDataFromWidget(bool data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(data ? "true" : "false");
	}
}

void UIDataBindString::onDataFromWidget(int data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(toString(data));
	}
}

void UIDataBindString::onDataFromWidget(float data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(toString(data));
	}
}

void UIDataBindString::onDataFromWidget(const String& data)
{
	if (canWriteData() && writeCallback) {
		writeCallback(data);
	}
}
