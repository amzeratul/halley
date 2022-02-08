#include "halley/bytes/serialization_dictionary.h"
#include "halley/data_structures/config_node.h"
#include "halley/support/logger.h"

using namespace Halley;

SerializationDictionary::SerializationDictionary()
{
}

SerializationDictionary::SerializationDictionary(const ConfigNode& config)
{
	size_t idx = 0;
	for (const auto& entry: config["highPriority"].asSequence()) {
		addEntry(idx++, entry.asString());
	}
	if (idx >= 128) {
		throw Exception("Too many high priority strings", 0);
	}

	idx = 128;
	for (const auto& entry: config["lowPriority"].asSequence()) {
		addEntry(idx++, entry.asString());
	}
}

std::optional<size_t> SerializationDictionary::stringToIndex(const String& string)
{
	auto iter = indices.find(string);
	if (iter == indices.end()) {
		return {};
	} else {
		return iter->second;
	}
}

const String& SerializationDictionary::indexToString(size_t index)
{
	return strings.at(index);
}

void SerializationDictionary::addEntry(String str)
{
	addEntry(strings.size(), std::move(str));
}

void SerializationDictionary::addEntry(size_t idx, String str)
{
	if (indices.find(str) != indices.end()) {
		throw Exception("Duplicated string in serialization dictionary: " + str, 0);
	}
	indices[str] = static_cast<int>(idx);
	
	if (idx == strings.size()) {
		strings.push_back(std::move(str));
	} else {
		strings.resize(idx + 1);
		strings.back() = std::move(str);
	}
}

void SerializationDictionary::addEntries(gsl::span<const String> strings)
{
	for (const auto& str: strings) {
		addEntry(str);
	}
}

void SerializationDictionary::setLogMissingStrings(bool enabled, int min, int freq)
{
	logMissingStrings = enabled;
	missingMin = min;
	missingFreq = freq;
}

void SerializationDictionary::notifyMissingString(const String& string)
{
	if (!logMissingStrings) {
		return;
	}

	const int cur = ++missing[string];
	if (cur >= missingMin && (cur == missingMin || cur % missingFreq == 0)) {
		Logger::logWarning("Serialization dictionary missing string: " + string + " (" + toString(cur) + "x)");
	}
}
