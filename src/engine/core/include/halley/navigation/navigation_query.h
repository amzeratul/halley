#pragma once

#include "world_position.h"
#include "halley/data_structures/config_node.h"
#include "halley/text/enum_names.h"

namespace Halley {
	class NavigationQuery {
	public:
		enum class PostProcessingType {
			None,
			Simple,
			Aggressive
		};

		enum class QuantizationType {
			None,
			Quantize8Way,
			Quantize8WayIsometric,
		};

		WorldPosition from;
		WorldPosition to;
		PostProcessingType postProcessingType;
		QuantizationType quantizationType;

		NavigationQuery();
		NavigationQuery(WorldPosition from, WorldPosition to, PostProcessingType postProcessing, QuantizationType quantizationType);
		explicit NavigationQuery(const ConfigNode& node);

		ConfigNode toConfigNode() const;
		String toString() const;

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

	template <>
	struct EnumNames<NavigationQuery::QuantizationType> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"none",
				"quantize8Way",
				"quantize8WayIsometric"
			}};
		}
	};
}
