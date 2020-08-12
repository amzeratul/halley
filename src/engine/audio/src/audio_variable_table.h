#pragma once
#include <unordered_map>
#include <halley/text/halleystring.h>

namespace Halley {
	class String;

	// TODO: this class is not very efficient for something in audio engine
	// Variables should probably be replaced by ints identifying them
	class AudioVariableTable {
    public:
		void set(const String& name, float value);
		float get(const String& name);

	private:
		std::unordered_map<String, float> variables;
    };
}
