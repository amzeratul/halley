#pragma once
#include "halley/bytes/byte_serializer.h"
#include "halley/data_structures/config_node.h"
#include "halley/text/string_converter.h"

namespace Halley {

	enum class AudioAttenuationCurve {
		Linear,
		Exponential
	};

	template <>
	struct EnumNames<AudioAttenuationCurve> {
		constexpr std::array<const char*, 2> operator()() const {
			return{{
				"linear",
				"exponential"
			}};
		}
	};
	
	class AudioAttenuation {
	public:
		float referenceDistance = 100.0f;
		float maximumDistance = 200.0f;
		AudioAttenuationCurve curve = AudioAttenuationCurve::Linear;

		AudioAttenuation() = default;
		AudioAttenuation(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

}
