#include "halley/input/input_keys.h"

#include "halley/support/logger.h"

using namespace Halley;

String KeyCodes::toString(KeyCode code)
{
	// TODO
	return "";
}

KeyCode KeyCodes::fromString(const String& str)
{
	if (auto result = tryFromString(str)) {
		return *result;
	} else {
		Logger::logError("Unable to convert \"" + str + "\" to keycode.");
		return KeyCode::Unknown;
	}
}

std::optional<KeyCode> KeyCodes::tryFromString(const String& str)
{
	// TODO
	return {};
}
