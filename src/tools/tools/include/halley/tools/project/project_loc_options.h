#pragma once

#include "halley/text/string_converter.h"
#include <array>

namespace Halley {
	class ConfigNode;

	enum class LocPriority : uint8_t {
		Lowest,
		Low,
		Normal,
		High,
		Highest
	};

	template <>
	struct EnumNames<LocPriority> {
		constexpr std::array<const char*, 5> operator()() const {
			return{{
				"lowest",
				"low",
				"normal",
				"high",
				"highest"
			}};
		}
	};

	class LocalisationFilterRules {
	public:
		LocPriority minPriorityForReady = LocPriority::High;

		LocalisationFilterRules() = default;
		LocalisationFilterRules(const ConfigNode& node);
	};
}
