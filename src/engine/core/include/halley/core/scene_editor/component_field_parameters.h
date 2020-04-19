#pragma once
#include <vector>

namespace Halley {
	class String;
	class ConfigNode;
	
	struct ComponentFieldParameters {
		ComponentFieldParameters(const String& componentName, const String& fieldName, const String& defaultValue, ConfigNode& componentData, const std::vector<String>& componentNames);

		const String& componentName;
		const String& fieldName;
		const String& defaultValue;
		ConfigNode& componentData;
		const std::vector<String>& componentNames;
	};
}
