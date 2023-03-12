#pragma once

#include <cstdint>

namespace Halley {
	class String;

	struct HalleyVersion {
		uint8_t major;
		uint8_t minor;
		uint16_t revision;

		String toString() const;
	};

	uint32_t getHalleyDLLAPIVersion();
	HalleyVersion getHalleyVersion();
}
