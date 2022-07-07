#pragma once
#include <functional>
#include <optional>

#include "halley/data_structures/config_node.h"
#include "halley/data_structures/vector.h"
#include "halley/text/halleystring.h"

namespace Halley {
	class ConfigNode;

	class ComponentDataRetriever {
	public:
		using Retriever = std::function<ConfigNode&(bool)>;
		
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
		ComponentFieldParameters(String componentName, ComponentDataRetriever data, Vector<String> defaultValue = {}, Vector<String> typeParameters = {}, ConfigNode options = {});

		ComponentFieldParameters withSubIndex(size_t index, Vector<String> defaultValue, Vector<String> typeParameters = {}) const;
		ComponentFieldParameters withSubKey(const String& key, Vector<String> defaultValue, Vector<String> typeParameters = {}) const;
		ComponentFieldParameters withSubIndex(size_t index, String defaultValue = "", Vector<String> typeParameters = {}) const;
		ComponentFieldParameters withSubKey(const String& key, String defaultValue = "", Vector<String> typeParameters = {}) const;
		ComponentFieldParameters withOptions(ConfigNode options) const;

		String getStringDefaultParameter(size_t n = 0) const;
		bool getBoolDefaultParameter(size_t n = 0) const;
		int getIntDefaultParameter(size_t n = 0) const;
		float getFloatDefaultParameter(size_t n = 0) const;

		void getDefaultParameter(float& dst, size_t n = 0) const { dst = getFloatDefaultParameter(n); }
		void getDefaultParameter(int& dst, size_t n = 0) const { dst = getIntDefaultParameter(n); }
		
		String componentName;
		ComponentDataRetriever data;
		Vector<String> defaultValue;
		Vector<String> typeParameters;
		ConfigNode options;
	};
}
