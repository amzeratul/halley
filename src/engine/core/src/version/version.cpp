#include "halley/version/version.h"
#include "../../../../../include/halley_version.hpp"
#include "halley/text/halleystring.h"
#include "halley/text/string_converter.h"

using namespace Halley;

bool HalleyVersion::operator==(const HalleyVersion& other) const
{
	return std::tuple(major, minor, revision) == std::tuple(other.major, other.minor, other.revision);
}

bool HalleyVersion::operator!=(const HalleyVersion& other) const
{
	return std::tuple(major, minor, revision) != std::tuple(other.major, other.minor, other.revision);
}

bool HalleyVersion::operator<(const HalleyVersion& other) const
{
	return std::tuple(major, minor, revision) < std::tuple(other.major, other.minor, other.revision);
}

bool HalleyVersion::operator<=(const HalleyVersion& other) const
{
	return std::tuple(major, minor, revision) <= std::tuple(other.major, other.minor, other.revision);
}

bool HalleyVersion::operator>(const HalleyVersion& other) const
{
	return std::tuple(major, minor, revision) > std::tuple(other.major, other.minor, other.revision);
}

bool HalleyVersion::operator>=(const HalleyVersion& other) const
{
	return std::tuple(major, minor, revision) >= std::tuple(other.major, other.minor, other.revision);
}

String HalleyVersion::toString() const
{
	return Halley::toString(static_cast<int>(major)) + "." + Halley::toString(static_cast<int>(minor)) + "." + Halley::toString(static_cast<int>(revision));
}

void HalleyVersion::parse(const String& string)
{
	const auto split = string.split('.');
	if (split.size() == 3) {
		major = split[0].toInteger();
		minor = split[1].toInteger();
		revision = split[2].toInteger();
	}
}

void HalleyVersion::parseHeader(gsl::span<const String> lines)
{
	major = 0;
	minor = 0;
	revision = 0;
	for (auto& line: lines) {
		if (line.startsWith("#define")) { 
			int value = 0;
			for (const auto& v: line.split(' ')) {
				if (v.isInteger()) {
					value = v.toInteger();
					break;
				}
			}
			if (line.contains("HALLEY_VERSION_MAJOR")) {
				major = value;
			} else if (line.contains("HALLEY_VERSION_MINOR")) {
				minor = value;
			} else if (line.contains("HALLEY_VERSION_REVISION")) {
				revision = value;
			}
		}
	}
}

bool HalleyVersion::isValid() const
{
	return major > 0;
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
