#include <utility>


#include "component_field_parameters.h"
#include "halley/file_formats/config_file.h"
using namespace Halley;

ComponentDataRetriever::ComponentDataRetriever(ConfigNode& componentData, String fieldName)
	: componentData(componentData)
	, name(fieldName)
{
	retriever = [&componentData, fieldName] () -> ConfigNode&
	{
		return componentData[fieldName];
	};
}

ComponentDataRetriever::ComponentDataRetriever(ConfigNode& componentData, String fieldName, Retriever retriever)
	: componentData(componentData)
	, name(std::move(fieldName))
	, retriever(std::move(retriever))
{}

ComponentDataRetriever ComponentDataRetriever::getSubIndex(size_t index) const
{
	auto r = retriever;
	return ComponentDataRetriever(componentData, name + "[" + toString(index) + "]", [retriever = std::move(r), index] () -> ConfigNode&
	{
		ConfigNode& node = retriever();
		if (node.getType() == ConfigNodeType::Sequence) {
			return node[index];
		} else if (index == 0 && node.getType() != ConfigNodeType::Map) {
			return node;
		} else {
			throw Exception("ConfigNode is not a sequence", HalleyExceptions::Entity);
		}
	});
}

ComponentDataRetriever ComponentDataRetriever::getSubKey(const String& key) const
{
	auto r = retriever;
	return ComponentDataRetriever(componentData, name + "[\"" + key + "\"]", [retriever = std::move(r), key] () -> ConfigNode&
	{
		return retriever()[key];
	});
}

ConfigNode& ComponentDataRetriever::getFieldData() const
{
	return retriever();
}

const String& ComponentDataRetriever::getName() const
{
	return name;
}

ComponentFieldParameters::ComponentFieldParameters(String componentName, std::vector<String> otherComponentNames, ComponentDataRetriever data, std::vector<String> defaultValue, std::vector<String> typeParameters)
	: componentName(std::move(componentName))
	, otherComponentNames(std::move(otherComponentNames))
	, data(std::move(data))
	, defaultValue(std::move(defaultValue))
	, typeParameters(std::move(typeParameters))
{}

ComponentFieldParameters ComponentFieldParameters::withSubIndex(size_t index, std::vector<String> defaultValue, std::vector<String> typeParameters) const
{
	return ComponentFieldParameters(componentName, otherComponentNames, data.getSubIndex(index), std::move(defaultValue), std::move(typeParameters));
}

ComponentFieldParameters ComponentFieldParameters::withSubKey(const String& key, std::vector<String> defaultValue, std::vector<String> typeParameters) const
{
	return ComponentFieldParameters(componentName, otherComponentNames, data.getSubKey(key), std::move(defaultValue), std::move(typeParameters));
}

ComponentFieldParameters ComponentFieldParameters::withSubIndex(size_t index, String defaultValue, std::vector<String> typeParameters) const
{
	return withSubIndex(index, defaultValue.isEmpty() ? std::vector<String>() : std::vector<String>{std::move(defaultValue)}, std::move(typeParameters));
}

ComponentFieldParameters ComponentFieldParameters::withSubKey(const String& key, String defaultValue, std::vector<String> typeParameters) const
{
	return withSubKey(key, defaultValue.isEmpty() ? std::vector<String>() : std::vector<String>{std::move(defaultValue)}, std::move(typeParameters));
}

String ComponentFieldParameters::getStringDefaultParameter(size_t n) const
{
	if (n < defaultValue.size()) {
		return defaultValue[n];
	}
	return "";
}

bool ComponentFieldParameters::getBoolDefaultParameter(size_t n) const
{
	const auto& str = getStringDefaultParameter(n);
	if (str == "true") {
		return true;
	}
	return false;
}

int ComponentFieldParameters::getIntDefaultParameter(size_t n) const
{
	const auto& str = getStringDefaultParameter(n);
	if (str.isInteger()) {
		return str.toInteger();
	}
	return 0;
}

float ComponentFieldParameters::getFloatDefaultParameter(size_t n) const
{
	const auto& str = getStringDefaultParameter(n);
	if (str.isNumber()) {
		return str.toFloat();
	}
	return 0.0f;
}
