#pragma once

#include <cstdint>

namespace Halley {
	class String;

	struct HalleyVersion {
		uint8_t major;
		uint8_t minor;
		uint16_t revision;

		bool operator==(const HalleyVersion& other) const;
		bool operator!=(const HalleyVersion& other) const;
		bool operator<(const HalleyVersion& other) const;
		bool operator<=(const HalleyVersion& other) const;
		bool operator>(const HalleyVersion& other) const;
		bool operator>=(const HalleyVersion& other) const;

		String toString() const;
		void parse(const String& string);
	};

	uint32_t getHalleyDLLAPIVersion();
	HalleyVersion getHalleyVersion();
}
