#pragma once

#include "halley/data_structures/config_node.h"
#include "halley/text/string_converter.h"

namespace Halley {
	class NavigationQuery {
	public:
		enum class PostProcessingType {
			None,
			Simple,
			Aggressive
		};

		Vector2f from;
		int fromSubWorld = 0;
		Vector2f to;
		int toSubWorld = 0;
		PostProcessingType postProcessingType;

		NavigationQuery();
		NavigationQuery(Vector2f from, int fromSubWorld, Vector2f to, int toSubWorld, PostProcessingType postProcessing);
		explicit NavigationQuery(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		bool operator==(const NavigationQuery& other) const;
		bool operator!=(const NavigationQuery& other) const;
	};

	template <>
	struct EnumNames<NavigationQuery::PostProcessingType> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"none",
				"simple",
				"aggressive"
			}};
		}
	};
}
