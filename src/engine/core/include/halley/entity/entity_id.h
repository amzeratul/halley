#pragma once

#include <cstdint>

namespace Halley {
	class ConfigNode;
	class EntitySerializationContext;
	class World;
	class Serializer;
	class Deserializer;
	class String;

	struct alignas(8) EntityId {
		int64_t value;

		EntityId() : value(-1) {}
		explicit EntityId(int64_t value) : value(value) {}
		explicit EntityId(const String& str);
		EntityId(const ConfigNode& node, const EntitySerializationContext& context);

		ConfigNode toConfigNode(const EntitySerializationContext& context) const;

		bool isValid() const { return value != -1; }
		bool operator==(const EntityId& other) const { return value == other.value; }
		bool operator!=(const EntityId& other) const { return value != other.value; }
		bool operator<(const EntityId& other) const { return value < other.value; }
		bool operator>(const EntityId& other) const { return value > other.value; }
		bool operator<=(const EntityId& other) const { return value <= other.value; }
		bool operator>=(const EntityId& other) const { return value >= other.value; }

		operator bool() const { return isValid(); }

		String toString() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};
}


namespace std {
	template<>
	struct hash<Halley::EntityId>
	{
		size_t operator()(const Halley::EntityId& v) const noexcept
		{
			return std::hash<int64_t>()(v.value);
		}
	};
}

