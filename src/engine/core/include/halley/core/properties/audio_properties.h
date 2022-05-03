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

		const String& getId() const;
		void setId(String value);
		gsl::span<const String> getValues() const;
		gsl::span<String> getValues();

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

		const String& getId() const;
		void setId(String value);
		Range<float> getRange() const;
		Range<float>& getRange();

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

		const String& getId() const;
		void setId(String value);
		gsl::span<const AudioBusProperties> getChildren() const;
		gsl::span<AudioBusProperties> getChildren();

		void collectBusIds(Vector<String>& output) const;

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

		Vector<String> getSwitchIds() const;
		Vector<String> getVariableIds() const;
		Vector<String> getBusIds() const;

		const AudioSwitchProperties* tryGetSwitch(const String& id) const;
		const AudioVariableProperties* tryGetVariable(const String& id) const;

	private:
		Vector<AudioSwitchProperties> switches;
		Vector<AudioVariableProperties> variables;
		Vector<AudioBusProperties> buses;

		void getBusIds(Vector<String>& result) const;
	};
}
