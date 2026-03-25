#pragma once

#include "halley/text/string_converter.h"
#include <array>

namespace Halley {
	class I18NLanguage;
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
		constexpr auto operator()() const {
			return std::to_array({
				"lowest",
				"low",
				"normal",
				"high",
				"highest"
			});
		}
	};

	class LocalisationFilterRules {
	public:
		LocalisationFilterRules() = default;
		LocalisationFilterRules(const ConfigNode& node);

		LocPriority getMinPriorityForReady(const I18NLanguage& language) const;

	private:
		HashMap<String, LocPriority> minPriorityForReady;
	};
}
