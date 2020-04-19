#include "scene_editor/component_field_parameters.h"
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
		return retriever()[index];
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

ComponentFieldParameters::ComponentFieldParameters(const String& componentName, ComponentDataRetriever data, const String& defaultValue, const std::vector<String>& componentNames)
	: componentName(componentName)
	, data(std::move(data))
	, defaultValue(defaultValue)
	, componentNames(componentNames)
{}
