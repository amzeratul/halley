#pragma once

#include "halley/text/halleystring.h"
#include "halley/utils/utils.h"
#include <gsl/gsl>
#include <array>
#include <optional>

#include "halley/data_structures/hash_map.h"
#include "halley/bytes/config_node_serializer_base.h"

namespace Halley {
	class ConfigNode;
	class Deserializer;
	class Serializer;

	class UUID {
    public:

		constexpr UUID()
		{
			qwords[0] = 0;
			qwords[1] = 0;
		}

		UUID(std::array<Byte, 16> b)
		{
			memcpy(qwords.data(), b.data(), 16);
		}

		UUID(gsl::span<const std::byte> b)
		{
			if (b.size_bytes() < 16) [[unlikely]] {
				qwords[0] = 0;
				qwords[1] = 0;
				memcpy(qwords.data(), b.data(), std::min(b.size_bytes(), size_t(16)));
			} else {
				memcpy(qwords.data(), b.data(), 16);
			}
		}

		UUID(const Bytes& b)
		{
			if (b.size() < 16) [[unlikely]] {
				qwords[0] = 0;
				qwords[1] = 0;
				memcpy(qwords.data(), b.data(), std::min(b.size(), size_t(16)));
			} else {
				memcpy(qwords.data(), b.data(), 16);
			}
		}

        explicit UUID(std::string_view str);
        explicit UUID(const ConfigNode& node);

        [[nodiscard]] static bool isUUID(std::string_view str);
        [[nodiscard]] static std::optional<UUID> tryParse(std::string_view str);

		[[nodiscard]] constexpr bool operator==(const UUID& other) const
		{
			return qwords == other.qwords;
		}

		[[nodiscard]] constexpr bool operator!=(const UUID& other) const
		{
			return qwords != other.qwords;
		}

		[[nodiscard]] constexpr bool operator<(const UUID& other) const
		{
			return qwords < other.qwords;
		}

		[[nodiscard]] UUID operator^(const UUID& other) const
		{
			return xorUUIDs(*this, other);
		}

		String toString() const;
        ConfigNode toConfigNode() const;

        [[nodiscard]] static UUID generate();

		[[nodiscard]] static UUID xorUUIDs(const UUID& one, const UUID& two)
		{
			UUID result;
			for (size_t i = 0; i < result.qwords.size(); i++) {
				result.qwords[i] = one.qwords[i] ^ two.qwords[i];
			}
			result.setVersionBits();
			return result;
		}

		[[nodiscard]] constexpr bool isValid() const
		{
			for (size_t i = 0; i < qwords.size(); ++i) {
				if (qwords[i] != 0) {
					return true;
				}
			}
			return false;
		}

        gsl::span<const std::byte> getBytes() const { return gsl::as_bytes(gsl::span<const uint64_t>(qwords)); }
		gsl::span<std::byte> getWriteableBytes() { return gsl::as_writable_bytes(gsl::span<uint64_t>(qwords)); }
        gsl::span<const uint64_t> getUint64Bytes() const { return qwords; }

    	void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

    private:
        std::array<uint64_t, 2> qwords;

		void setVersionBits()
		{
			auto* bs = reinterpret_cast<unsigned char*>(qwords.data());

			bs[6] = (bs[6] & 0b00001111) | (4 << 4); // Version 4
			bs[8] = (bs[8] & 0b00111111) | (0b10 << 6); // Variant 1
		}
	};

    template <>
    class ConfigNodeSerializer<UUID> {
    public:
        ConfigNode serialize(UUID id, const EntitySerializationContext& context);
        UUID deserialize(const EntitySerializationContext& context, const ConfigNode& node);
    };
}

template<>
struct std::hash<Halley::UUID>
{
    constexpr std::size_t operator() (const Halley::UUID& uuid) const noexcept
    {
        return Halley::Hash::hash(gsl::as_bytes(uuid.getUint64Bytes()));
    }
};

namespace natvis {
    struct x4lo {
    	uint8_t v: 4;
    	uint8_t _: 4;
    };
    struct x4hi {
	    uint8_t _ : 4;
    	uint8_t v : 4;
    };
    struct x8 {
	    uint8_t _;
    };
	struct x16 {
	    uint16_t _;
    };
    struct x32 {
	    uint32_t _;
    };
	struct x48 {
	    uint32_t b0;
		uint16_t b1;
    };
}
