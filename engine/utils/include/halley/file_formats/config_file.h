#pragma once

#include <map>
#include <vector>
#include "halley/text/halleystring.h"
#include <boost/variant.hpp>
#include "halley/resources/resource.h"

namespace Halley
{
	class ResourceLoader;
	class Serializer;
	class Deserializer;

	enum class ConfigNodeType
	{
		Undefined,
		Scalar,
		Sequence,
		Map
	};
	
	class ConfigNode
	{
	public:
		using MapType = std::map<String, ConfigNode>;
		using SequenceType = std::vector<ConfigNode>;
		using ScalarType = String;

		ConfigNode();
		ConfigNode(MapType&& entryMap);
		ConfigNode(SequenceType&& entryList);
		ConfigNode(ScalarType&& value);
		
		ConfigNode& operator=(MapType&& entryMap);
		ConfigNode& operator=(SequenceType&& entryList);
		ConfigNode& operator=(ScalarType&& value);
		ConfigNode& operator=(const MapType& entryMap);
		ConfigNode& operator=(const SequenceType& entryList);
		ConfigNode& operator=(const ScalarType& value);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		int asInt() const;
		float asFloat() const;
		bool asBool() const;
		String asString() const;

		int asInt(int defaultValue) const;
		float asFloat(float defaultValue) const;
		bool asBool(bool defaultValue) const;
		String asString(const String& defaultValue) const;

		const SequenceType& asSequence() const;
		const MapType& asMap() const;
		SequenceType& asSequence();
		MapType& asMap();

		bool hasKey(const String& key) const;

		ConfigNode& operator[](const String& key);
		ConfigNode& operator[](size_t idx);

		const ConfigNode& operator[](const String& key) const;
		const ConfigNode& operator[](size_t idx) const;

		SequenceType::iterator begin();
		SequenceType::iterator end();

		SequenceType::const_iterator begin() const;
		SequenceType::const_iterator end() const;

	private:
		ConfigNodeType type;
		boost::variant<ScalarType, SequenceType, MapType> contents;

		static ConfigNode undefinedConfigNode;
	};

	class ConfigFile : public Resource
	{
	public:
		ConfigNode& getRoot();
		const ConfigNode& getRoot() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		static std::unique_ptr<ConfigFile> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::ConfigFile; }

	private:
		ConfigNode root;
	};
}
