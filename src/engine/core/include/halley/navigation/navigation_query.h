#pragma once

#include "world_position.h"
#include "halley/data_structures/config_node.h"
#include "halley/text/enum_names.h"

namespace Halley {
	class NavigationQuery {
	public:
		enum class PostProcessingType : uint8_t {
			None,
			Normal
		};

		enum class QuantizationType : uint8_t {
			None,
			Quantize8Way,
			Quantize8WayIsometric,
		};

		class DebugData {
		public:
			String agentId;

			DebugData() = default;
			DebugData(String agentId);

			String toString() const;

			bool operator==(const DebugData& other) const = default;
		};

		WorldPosition from;
		WorldPosition to;
		PostProcessingType postProcessingType;
		QuantizationType quantizationType;
		float sideShift = 0;
		DebugData debugData;

		NavigationQuery();
		NavigationQuery(WorldPosition from, WorldPosition to, PostProcessingType postProcessing, QuantizationType quantizationType, float sideShift, DebugData debugData = {});
		explicit NavigationQuery(const ConfigNode& node);

		ConfigNode toConfigNode() const;
		void feedToHasher(Hash::Hasher& hasher) const;
		String toString() const;

		bool operator==(const NavigationQuery& other) const = default;
	};

	template <>
	struct EnumNames<NavigationQuery::PostProcessingType> {
		constexpr auto operator()() const {
			return std::to_array({
				"none",
				"simple",
				"aggressive"
			});
		}
	};

	template <>
	struct EnumNames<NavigationQuery::QuantizationType> {
		constexpr auto operator()() const {
			return std::to_array({
				"none",
				"quantize8Way",
				"quantize8WayIsometric"
			});
		}
	};
}
