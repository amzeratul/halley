#pragma once

#include <cstdint>
#include "halley/bytes/config_node_serializer.h"

namespace Halley {
	class World;

	struct alignas(8) EntityId {
		int64_t value;

		EntityId() : value(-1) {}
		explicit EntityId(int64_t value) : value(value) {}
		explicit EntityId(const String& str);

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

	template <>
    class ConfigNodeSerializer<EntityId> {
    public:
		ConfigNode serialize(EntityId id, const EntitySerializationContext& context);
		EntityId deserialize(const EntitySerializationContext& context, const ConfigNode& node);
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

