#pragma once
#include "halley/maths/range.h"
#include "halley/resources/resource.h"
#include "halley/resources/resource_data.h"

namespace Halley {
	class AudioSwitchProperties {
	public:
		AudioSwitchProperties() = default;
		AudioSwitchProperties(const ConfigNode& node);

		ConfigNode toConfigNode() const;
		
		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		String id;
		Vector<String> values;
	};

	class AudioVariableProperties {
	public:
		AudioVariableProperties() = default;
		AudioVariableProperties(const ConfigNode& node);

		ConfigNode toConfigNode() const;
		
		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		String id;
		Range<float> range;
	};

	class AudioBusProperties {
	public:
		AudioBusProperties() = default;
		AudioBusProperties(const ConfigNode& node);

		ConfigNode toConfigNode() const;
		
		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		String id;
		Vector<AudioBusProperties> children;
	};

	class AudioProperties {
	public:
		AudioProperties() = default;
		AudioProperties(const ConfigNode& node);

		ConfigNode toConfigNode() const;
		
		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		gsl::span<const AudioSwitchProperties> getSwitches() const;
		gsl::span<AudioSwitchProperties> getSwitches();
		gsl::span<const AudioVariableProperties> getVariables() const;
		gsl::span<AudioVariableProperties> getVariables();
		gsl::span<const AudioBusProperties> getBuses() const;
		gsl::span<AudioBusProperties> getBuses();

	private:
		Vector<AudioSwitchProperties> switches;
		Vector<AudioVariableProperties> variables;
		Vector<AudioBusProperties> buses;
	};
}
