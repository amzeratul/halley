#include "scene_editor/component_field_parameters.h"
using namespace Halley;

ComponentFieldParameters::ComponentFieldParameters(const String& componentName, const String& fieldName, const String& defaultValue, ConfigNode& componentData, const std::vector<String>& componentNames)
	: componentName(componentName)
	, fieldName(fieldName)
	, defaultValue(defaultValue)
	, componentData(componentData)
	, componentNames(componentNames)
{}
