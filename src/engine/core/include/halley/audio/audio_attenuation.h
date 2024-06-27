#pragma once
#include "halley/bytes/byte_serializer.h"
#include "halley/data_structures/config_node.h"
#include "halley/text/string_converter.h"

namespace Halley {

	enum class AudioAttenuationCurve {
		None,
		Linear,
		InvDistance,
		Exponential
	};

	template <>
	struct EnumNames<AudioAttenuationCurve> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"none",
				"linear",
				"invDistance",
				"exponential"
			}};
		}
	};
	
	class AudioAttenuation {
	public:
		float referenceDistance = 200.0f;
		float maximumDistance = 400.0f;
		float rollOffFactor = 1;
		AudioAttenuationCurve curve = AudioAttenuationCurve::Linear;

		AudioAttenuation() = default;
		AudioAttenuation(const ConfigNode& node);
		AudioAttenuation(float refDistance, float maxDistance, float rollOffFactor = 1.0f, AudioAttenuationCurve curve = AudioAttenuationCurve::Linear);

		ConfigNode toConfigNode() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		float getProximity(float distance) const;
	};

}
