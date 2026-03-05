#include "halley/support/assert.h"
#include "halley/support/exception.h"
#include "halley/text/string_converter.h"

using namespace Halley;

void Halley::halleyAssert(std::string_view str, std::string_view filename, int line)
{
	throw Exception("Assert (" + String(str) + ") failed at " + filename + ":" + toString(line), HalleyExceptions::Assert);
}
