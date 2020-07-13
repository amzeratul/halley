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
    	bool isValid() const;

        gsl::span<const gsl::byte> getBytes() const;
		gsl::span<gsl::byte> getBytes();

    	void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

    private:
		std::array<Byte, 16> bytes;
    };
}
