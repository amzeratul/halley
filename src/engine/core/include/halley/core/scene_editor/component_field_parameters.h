#pragma once
#include <functional>
#include <vector>
#include "halley/text/halleystring.h"

namespace Halley {
	class ConfigNode;

	class ComponentDataRetriever {
	public:
		using Retriever = std::function<ConfigNode&()>;
		
		ComponentDataRetriever(ConfigNode& componentData, String fieldName);
		ComponentDataRetriever getSubIndex(size_t index) const;
		ComponentDataRetriever getSubKey(const String& key) const;
		
		ConfigNode& getFieldData() const;
		const String& getName() const;

	private:
		ConfigNode& componentData;
		String name;
		Retriever retriever;

		ComponentDataRetriever(ConfigNode& componentData, String fieldName, Retriever retriever);
	};
	
	class ComponentFieldParameters {
	public:
		ComponentFieldParameters(String componentName, std::vector<String> otherComponentNames, ComponentDataRetriever data, String defaultValue = "", std::vector<String> typeParameters = {});

		ComponentFieldParameters withSubIndex(size_t index, String defaultValue = "", std::vector<String> typeParameters = {}) const;
		ComponentFieldParameters withSubKey(const String& key, String defaultValue = "", std::vector<String> typeParameters = {}) const;

		String componentName;
		std::vector<String> otherComponentNames;
		ComponentDataRetriever data;
		String defaultValue;
		std::vector<String> typeParameters;
	};
}
