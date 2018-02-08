#pragma once

#include "halley/text/halleystring.h"
#include <memory>
#include <map>

namespace Halley
{
	class Deserializer;
	class Serializer;
	class ResourceDataStatic;

	class Metadata
	{
	public:
		Metadata();
		Metadata(const Metadata& other) = default;
		~Metadata();

		bool hasKey(String key) const;

		bool getBool(String key) const;
		int getInt(String key) const;
		float getFloat(String key) const;
		String getString(String key) const;

		bool getBool(String key, bool defaultValue) const;
		int getInt(String key, int defaultValue) const;
		float getFloat(String key, float defaultValue) const;
		String getString(String key, String defaultValue) const;

		void set(String key, bool value);
		void set(String key, int value);
		void set(String key, float value);
		void set(String key, const char* value);
		void set(String key, const std::string& value);
		void set(String key, const String& value);
		
		static std::unique_ptr<Metadata> fromBinary(ResourceDataStatic& data);
		
		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		bool operator==(const Metadata& rhs) const;
		bool operator!=(const Metadata& rhs) const;

	private:
		std::map<String, String> entries;
	};
}
