#pragma once

#include <map>
#include <vector>
#include "halley/text/halleystring.h"
#include "halley/maths/vector2.h"
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
		String,
		Sequence,
		Map,
		Int,
		Float,
		Int2,
		Float2,
		Bytes
	};
	
	class ConfigNode
	{
	public:
		using MapType = std::map<String, ConfigNode>;
		using SequenceType = std::vector<ConfigNode>;

		ConfigNode();
		ConfigNode(const ConfigNode& other);
		ConfigNode(ConfigNode&& other);
		ConfigNode(MapType&& entryMap);
		ConfigNode(SequenceType&& entryList);
		ConfigNode(String&& value);
		ConfigNode(bool value);
		ConfigNode(int value);
		ConfigNode(float value);
		ConfigNode(Vector2i value);
		ConfigNode(Vector2f value);
		ConfigNode(Bytes&& value);

		~ConfigNode();
		
		ConfigNode& operator=(const ConfigNode& other);
		ConfigNode& operator=(ConfigNode&& other);
		ConfigNode& operator=(bool value);
		ConfigNode& operator=(int value);
		ConfigNode& operator=(float value);
		ConfigNode& operator=(Vector2i value);
		ConfigNode& operator=(Vector2f value);

		ConfigNode& operator=(MapType&& entryMap);
		ConfigNode& operator=(SequenceType&& entryList);
		ConfigNode& operator=(String&& value);
		ConfigNode& operator=(Bytes&& value);

		ConfigNode& operator=(const MapType& entryMap);
		ConfigNode& operator=(const SequenceType& entryList);
		ConfigNode& operator=(const String& value);
		ConfigNode& operator=(const Bytes& value);

		ConfigNodeType getType() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		int asInt() const;
		float asFloat() const;
		bool asBool() const;
		Vector2i asVector2i() const;
		Vector2f asVector2f() const;
		String asString() const;
		const Bytes& asBytes() const;

		int asInt(int defaultValue) const;
		float asFloat(float defaultValue) const;
		bool asBool(bool defaultValue) const;
		String asString(const String& defaultValue) const;
		Vector2i asVector2i(Vector2i defaultValue) const;
		Vector2f asVector2f(Vector2f defaultValue) const;

		const SequenceType& asSequence() const;
		const MapType& asMap() const;
		SequenceType& asSequence();
		MapType& asMap();

		bool hasKey(const String& key) const;
		void removeKey(const String& key);

		ConfigNode& operator[](const String& key);
		ConfigNode& operator[](size_t idx);

		const ConfigNode& operator[](const String& key) const;
		const ConfigNode& operator[](size_t idx) const;

		SequenceType::iterator begin();
		SequenceType::iterator end();

		SequenceType::const_iterator begin() const;
		SequenceType::const_iterator end() const;

		void reset();

	private:
		union {
			void* ptrData;
			int intData;
			float floatData;
			Vector2i vec2iData;
			Vector2f vec2fData;
		};
		ConfigNodeType type;

		static ConfigNode undefinedConfigNode;

		template <typename T> void deserializeContents(Deserializer& s)
		{
			T v;
			s >> v;
			*this = std::move(v);
		}
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
