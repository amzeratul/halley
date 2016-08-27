#include <cstring>
#include <string>
#include "halley/file_formats/serializer.h"
#include "halley/text/halleystring.h"

using namespace Halley;

#ifndef _MSC_VER
void memcpy_s(void* dst, size_t size, const void* src, size_t)
{
	memcpy(dst, src, size);
}
#endif

Serializer::Serializer()
	: dryRun(true)
{}

Serializer::Serializer(gsl::span<gsl::byte> dst)
	: dryRun(false)
{}

Serializer& Serializer::operator<<(const std::string& str)
{
	size_t sz = str.size();
	*this << sz;
	*this << gsl::as_bytes(gsl::span<const char>(str.data(), sz));
	return *this;
}

Serializer& Serializer::operator<<(const String& str)
{
	return (*this << str.cppStr());
}

Serializer& Serializer::operator<<(gsl::span<const gsl::byte> span)
{
	if (!dryRun) {
		memcpy(dst.data() + size, span.data(), span.size_bytes());
	}
	size += span.size_bytes();
	return *this;
}

Deserializer::Deserializer(gsl::span<const gsl::byte> src)
	: pos(0)
	, src(src)
{
}

Deserializer& Deserializer::operator>>(std::string& str)
{
	size_t sz;
	*this >> sz;
	str = std::string(reinterpret_cast<const char*>(src.data() + pos), sz);
	pos += sz;
	return *this;
}

Deserializer& Deserializer::operator>>(gsl::span<gsl::byte>& span)
{
	memcpy(span.data(), src.data() + pos, span.size_bytes());
	pos += span.size_bytes();
	return *this;
}
