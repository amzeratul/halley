#include "audio_variable_table.h"

using namespace Halley;

void AudioVariableTable::set(const String& name, float value)
{
	variables[name] = value;
}

float AudioVariableTable::get(const String& name) const
{
	const auto iter = variables.find(name);
	if (iter != variables.end()) {
		return iter->second;
	}
	return 0;
}
