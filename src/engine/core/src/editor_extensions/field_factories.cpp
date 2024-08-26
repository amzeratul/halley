#include "halley/editor_extensions/field_factories.h"
#include "halley/editor_extensions/component_editor_context.h"
#include "halley/editor_extensions/component_field_parameters.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_sizer.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/widgets/ui_dropdown_multi_select.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

std::shared_ptr<IUIElement> BaseEnumFieldFactory::createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars)
{
	const auto data = pars.data;
	const auto& values = getValues(data);

	auto& fieldData = data.getFieldData();
	const auto defaultValue = pars.getStringDefaultParameter().split("::").back();
	const auto& origValue = fieldData.asString("");
	const auto& value = fieldData.asString(getDefaultValue(values, defaultValue));

	const auto& dropStyle = context.getUIFactory().getStyle("dropdown");

	const auto sizer = std::make_shared<UISizer>();
	auto dropdown = std::make_shared<UIDropdown>("enum", dropStyle);
	dropdown->setOptions(values);
	sizer->add(dropdown, 1);

	dropdown->bindData("enum", value, [&context, data](String newVal) {
		auto& node = data.getWriteableFieldData();
		node = ConfigNode(std::move(newVal));
		context.onEntityUpdated();
	});

	if (origValue != value && defaultValue.isEmpty()) {
		auto& node = data.getWriteableFieldData();
		node = ConfigNode(value);
		context.onEntityUpdated();
	}

	if (const auto dependentField = getDependentField()) {
		sizer->add(std::make_shared<DependencyObserver>(data, *dependentField, data.getParentFieldData()[*dependentField].asString(""), [this, dropdown, &context] (const ComponentDataRetriever& data)
		{
			dropdown->setOptions(getValues(data));
			data.getWriteableFieldData() = ConfigNode(dropdown->getSelectedOptionId());
			context.onEntityUpdated();
		}));
	}

	return sizer;
}

String BaseEnumFieldFactory::getDefaultValue(const Vector<String>& values, const String& requestedValue) const
{
	auto lc = requestedValue.asciiLower();

	for (auto& value: values) {
		if (value.asciiLower() == lc) {
			return value;
		}
	}
	return values.empty() ? "" : values.front();
}

std::optional<String> BaseEnumFieldFactory::getDependentField() const
{
	return std::nullopt;
}

DependencyObserver::DependencyObserver(ComponentDataRetriever data, String fieldName, String value, std::function<void(const ComponentDataRetriever&)> refreshCallback)
	: UIWidget()
	, data(std::move(data))
	, fieldName(std::move(fieldName))
	, value(std::move(value))
	, refreshCallback(std::move(refreshCallback))
{
}

void DependencyObserver::update(Time t, bool moved)
{
	const auto& curValue = data.getParentFieldData()[fieldName].asString("");
	if (curValue != value) {
		value = curValue;
		refreshCallback(data);
	}
}

EnumFieldFactory::EnumFieldFactory(String name, Vector<String> values, String defaultValue)
	: fieldName(std::move(name))
	, defaultValue(std::move(defaultValue))
	, values(std::move(values))
{
}

String EnumFieldFactory::getFieldType()
{
	return fieldName;
}

Vector<String> EnumFieldFactory::getValues(const ComponentDataRetriever& data) const
{
	return values;
}

String EnumFieldFactory::getDefaultValue(const Vector<String>& values, const String& requestedValue) const
{
	auto lc = requestedValue.asciiLower();

	for (auto& value: values) {
		if (value.asciiLower() == lc) {
			return value;
		}
	}
	return defaultValue;
}


EnumIntFieldFactory::EnumIntFieldFactory(String name, Vector<String> names, Vector<int> values)
	: fieldName(std::move(name))
	, names(std::move(names))
	, values(std::move(values))
{
}

std::shared_ptr<IUIElement> EnumIntFieldFactory::createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars)
{
	const auto data = pars.data;

	auto& fieldData = data.getFieldData();
	const auto& defaultValue = pars.getIntDefaultParameter();
	const auto& value = fieldData.asInt(defaultValue);

	const auto& dropStyle = context.getUIFactory().getStyle("dropdown");

	auto dropdown = std::make_shared<UIDropdown>("enum", dropStyle);
	dropdown->setOptions(names);

	const auto valueIter = std_ex::find(values, value);
	size_t idx;
	if (valueIter != values.end()) {
		idx = valueIter - values.begin();
	} else {
		idx = 0;
		auto& node = data.getWriteableFieldData();
		node = ConfigNode(values[idx]);
		context.onEntityUpdated();
	}

	dropdown->bindData("enum", names[idx], [&context, data, names = names, values = values](String newVal) {
		auto& node = data.getWriteableFieldData();
		auto idx = std_ex::find(names, newVal) - names.begin();
		node = ConfigNode(values[idx]);
		context.onEntityUpdated();
	});
	
	return dropdown;
}

String EnumIntFieldFactory::getFieldType()
{
	return fieldName;
}



EnumIntMaskFieldFactory::EnumIntMaskFieldFactory(String name, Vector<String> names, Vector<int> values)
	: fieldName(std::move(name))
	, names(std::move(names))
	, values(std::move(values))
{}

std::shared_ptr<IUIElement> EnumIntMaskFieldFactory::createField(const ComponentEditorContext& context, const ComponentFieldParameters& pars)
{
	const auto data = pars.data;

	auto& fieldData = data.getFieldData();
	const auto& dropStyle = context.getUIFactory().getStyle("dropdown");
		
	auto dropdown = std::make_shared<UIDropdownMultiSelect>("dropdown", dropStyle);

	const auto initialValue = fieldData.asInt(pars.getIntDefaultParameter());

	Vector<UIDropdown::Entry> options;
	Vector<int> curOptions;
	for (auto i = 0; i < static_cast<int>(names.size()); i++) {
		const auto& id = names.at(i);
		const int value = values.at(i);

		options.emplace_back(UIDropdown::Entry{ id, id, {}, value });
		if ((value & initialValue) != 0) {
			curOptions.push_back(i);
		}
	}
	
	dropdown->setOptions(options, curOptions);

	dropdown->bindData("dropdown", initialValue, [&context, data](int newVal) {
		auto& node = data.getWriteableFieldData();
		node = ConfigNode(newVal);
		context.onEntityUpdated();
	});

	return dropdown;
}

String EnumIntMaskFieldFactory::getFieldType()
{
	return fieldName;
}



ConfigDBFieldFactory::ConfigDBFieldFactory(String name, Vector<String> values)
	: fieldName(std::move(name))
	, values(std::move(values))
{
}

String ConfigDBFieldFactory::getFieldType()
{
	return fieldName;
}

Vector<String> ConfigDBFieldFactory::getValues(const ComponentDataRetriever& data) const
{
	return values;
}
