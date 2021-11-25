#pragma once
#include <functional>
#include <optional>
#include <vector>
#include "halley/text/halleystring.h"

namespace Halley {
	class ConfigNode;

	class ComponentDataRetriever {
	public:
		using Retriever = std::function<ConfigNode&()>;
		
		ComponentDataRetriever(ConfigNode& componentData, String fieldName, String labelName);
		ComponentDataRetriever getSubIndex(size_t index) const;
		ComponentDataRetriever getSubKey(const String& key) const;
		
		const ConfigNode& getFieldData() const;
		ConfigNode& getWriteableFieldData() const;
		const String& getName() const;
		const String& getLabelName() const;

	private:
		ConfigNode& componentData;
		String labelName;
		String name;
		Retriever retriever;

		ComponentDataRetriever(ConfigNode& componentData, String fieldName, String labelName, Retriever retriever);
	};
	
	class ComponentFieldParameters {
	public:
		ComponentFieldParameters(String componentName, ComponentDataRetriever data, std::vector<String> defaultValue = {}, std::vector<String> typeParameters = {});

		ComponentFieldParameters withSubIndex(size_t index, std::vector<String> defaultValue, std::vector<String> typeParameters = {}) const;
		ComponentFieldParameters withSubKey(const String& key, std::vector<String> defaultValue, std::vector<String> typeParameters = {}) const;
		ComponentFieldParameters withSubIndex(size_t index, String defaultValue = "", std::vector<String> typeParameters = {}) const;
		ComponentFieldParameters withSubKey(const String& key, String defaultValue = "", std::vector<String> typeParameters = {}) const;

		String getStringDefaultParameter(size_t n = 0) const;
		bool getBoolDefaultParameter(size_t n = 0) const;
		int getIntDefaultParameter(size_t n = 0) const;
		float getFloatDefaultParameter(size_t n = 0) const;

		void getDefaultParameter(float& dst, size_t n = 0) const { dst = getFloatDefaultParameter(n); }
		void getDefaultParameter(int& dst, size_t n = 0) const { dst = getIntDefaultParameter(n); }
		
		String componentName;
		ComponentDataRetriever data;
		std::vector<String> defaultValue;
		std::vector<String> typeParameters;
	};
}
