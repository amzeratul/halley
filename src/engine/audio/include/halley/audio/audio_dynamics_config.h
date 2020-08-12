#pragma once
#include <vector>

#include "halley/text/halleystring.h"

namespace Halley {
	class Deserializer;
	class Serializer;
	class ConfigNode;

	class AudioDynamicsConfig {
	public:
		class Variable {
		public:
			Variable();
			Variable(const ConfigNode& node);

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);

			float getValue(float variable) const;

			String name;
		};

		AudioDynamicsConfig();
		AudioDynamicsConfig(const ConfigNode& node);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		const std::vector<Variable>& getVolume() const;

	private:
		std::vector<Variable> volume;
	};
}
