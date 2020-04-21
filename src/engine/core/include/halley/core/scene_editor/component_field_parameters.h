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
		ComponentFieldParameters(const String& componentName, ComponentDataRetriever data, const String& defaultValue, const std::vector<String>& componentNames, const std::vector<String>& typeParameters);

		const String& componentName;
		ComponentDataRetriever data;
		const String& defaultValue;
		const std::vector<String>& componentNames;
		const std::vector<String>& typeParameters;
	};
}
