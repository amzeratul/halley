#include "halley/core/version/version.h"
#include "../../../../../include/halley_version.hpp"
#include "halley/text/halleystring.h"
#include "halley/text/string_converter.h"

using namespace Halley;

String HalleyVersion::toString() const
{
	return Halley::toString(static_cast<int>(major)) + "." + Halley::toString(static_cast<int>(minor)) + "." + Halley::toString(static_cast<int>(revision));
}

uint32_t Halley::getHalleyDLLAPIVersion()
{
	const auto version = getHalleyVersion();
	return (static_cast<uint32_t>(version.major) << 24) + (static_cast<uint32_t>(version.minor) << 16) + static_cast<uint32_t>(version.revision);
}

HalleyVersion Halley::getHalleyVersion()
{
	HalleyVersion version;
	version.major = HALLEY_VERSION_MAJOR;
	version.minor = HALLEY_VERSION_MINOR;
	version.revision = HALLEY_VERSION_REVISION;
	return version;
}
