#pragma once

#include "halley/text/halleystring.h"
#include <memory>
#include <map>

#include "halley/data_structures/config_node.h"

namespace Halley
{
	class Deserializer;
	class Serializer;
	class ResourceDataStatic;
	class ConfigNode;

	class Metadata
	{
	public:
		Metadata();
		Metadata(const Metadata& other) = default;
		Metadata(Metadata&& other) noexcept = default;
		~Metadata();

		Metadata& operator=(const Metadata& other) = default;
		Metadata& operator=(Metadata&& other) noexcept = default;

		bool hasKey(std::string_view key) const;

		bool getBool(std::string_view key) const;
		int getInt(std::string_view key) const;
		float getFloat(std::string_view key) const;
		String getString(std::string_view key) const;
		ConfigNode getValue(std::string_view key) const;

		bool getBool(std::string_view key, bool defaultValue) const;
		int getInt(std::string_view key, int defaultValue) const;
		float getFloat(std::string_view key, float defaultValue) const;
		String getString(std::string_view key, String defaultValue) const;
		const ConfigNode& getEntries() const;

		bool set(std::string_view key, std::string_view value);
		bool set(std::string_view key, const char* value);
		bool set(std::string_view key, const std::string& value);
		bool set(std::string_view key, const String& value);

		template <typename T>
		bool set(std::string_view key, T value)
		{
			static_assert(!std::is_same_v<T, String>);
			static_assert(!std::is_same_v<T, std::string>);
			static_assert(!std::is_same_v<T, std::string_view>);
			static_assert(!std::is_same_v<T, const char*>);

			auto& es = entries.asMap();
			const auto iter = es.find(key);
			if (iter != es.end()) {
				auto v = ConfigNode(value);
				if (iter->second == v) {
					return false;
				}
				iter->second = std::move(v);
			} else {
				es[key] = ConfigNode(std::move(value));
			}
			return true;
		}

		bool erase(std::string_view key);

		void convertToLatestVersion();

		static std::unique_ptr<Metadata> fromBinary(ResourceDataStatic& data);
		
		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		bool operator==(const Metadata& rhs) const;
		bool operator!=(const Metadata& rhs) const;

		String toString() const;
		ConfigNode toConfig() const;

	private:
		ConfigNode entries;
	};
}
