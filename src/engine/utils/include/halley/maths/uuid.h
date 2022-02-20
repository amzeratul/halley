#pragma once

#include "halley/text/halleystring.h"
#include "halley/utils/utils.h"
#include <gsl/gsl>
#include <array>

namespace Halley {
	class Deserializer;
	class Serializer;

	class UUID {
    public:
        UUID();
        UUID(std::array<Byte, 16> bytes);
        explicit UUID(gsl::span<const gsl::byte> bytes);
		explicit UUID(const Bytes& bytes);
        explicit UUID(const String& str);

        bool operator==(const UUID& other) const;
        bool operator!=(const UUID& other) const;
		bool operator<(const UUID& other) const;

		String toString() const;

        static UUID generate();
        static UUID generateFromUUIDs(const UUID& one, const UUID& two);
    	bool isValid() const;

        gsl::span<const gsl::byte> getBytes() const;
		gsl::span<gsl::byte> getWriteableBytes();
        gsl::span<const uint64_t> getUint64Bytes() const;

    	void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

    private:
        union {
            std::array<uint64_t, 2> qwords;
            std::array<uint8_t, 16> bytes;
        };
	};
}

template<>
struct std::hash<Halley::UUID>
{
    std::size_t operator() (const Halley::UUID& uuid) const noexcept
    {
        const auto& bytes = uuid.getUint64Bytes();
        return bytes[0] ^ bytes[1];
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
