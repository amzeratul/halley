#pragma once

#include "halley/text/halleystring.h"
#include "halley/utils/utils.h"

namespace Halley {
    class UUID {
    public:
        UUID();
        UUID(std::array<Byte, 16> bytes);
        explicit UUID(const String& str);

        bool operator==(const UUID& other) const;
        bool operator!=(const UUID& other) const;
		bool operator<(const UUID& other) const;

		String toString() const;

		static UUID generate();

    private:
		std::array<Byte, 16> bytes;
    };
}
