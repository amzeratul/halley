#pragma once
#include <halley/text/halleystring.h>

#include "halley/data_structures/hash_map.h"

namespace Halley {
	class String;

	// TODO: this class is not very efficient for something in audio engine
	// Variables should probably be replaced by ints identifying them
	class AudioVariableTable {
    public:
		void set(const String& name, float value);
		float get(const String& name) const;

	private:
		HashMap<String, float> variables;
    };
}
