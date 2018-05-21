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

void UIDataBindInt::onDataFromWidget(int data)
{
	writeCallback(data);
}

void UIDataBindInt::onDataFromWidget(float data)
{
	writeCallback(lround(data));
}

void UIDataBindInt::onDataFromWidget(const String& data)
{
	writeCallback(data.toInteger());
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
