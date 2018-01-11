#include <cstring>
#include <string>
#include "halley/file/byte_serializer.h"
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
	, dst(dst)
{}

Serializer& Serializer::operator<<(const std::string& str)
{
	unsigned int sz = static_cast<unsigned int>(str.size());
	*this << sz;
	*this << gsl::as_bytes(gsl::span<const char>(str.data(), sz));
	return *this;
}

Serializer& Serializer::operator<<(const String& str)
{
	return (*this << str.cppStr());
}

Serializer& Serializer::operator<<(const Path& path)
{
	return (*this << path.string());
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

Deserializer::Deserializer(const Bytes& src)
	: pos(0)
	, src(gsl::as_bytes(gsl::span<const Halley::Byte>(src)))
{
}

Deserializer& Deserializer::operator>>(std::string& str)
{
	unsigned int sz;
	*this >> sz;

	ensureSufficientBytesRemaining(sz);

	Expects(sz < 100 * 1024 * 1024);
	str = std::string(reinterpret_cast<const char*>(src.data() + pos), sz);
	pos += sz;
	return *this;
}

Deserializer& Deserializer::operator>>(String& str)
{
	std::string s;
	*this >> s;
	str = s;
	return *this;
}

Deserializer& Deserializer::operator>>(Path& p)
{
	std::string s;
	*this >> s;
	p = s;
	return *this;
}

Deserializer& Deserializer::operator>>(gsl::span<gsl::byte>& span)
{
	Expects(span.size_bytes() > 0);

	ensureSufficientBytesRemaining(size_t(span.size_bytes()));

	memcpy(span.data(), src.data() + pos, span.size_bytes());
	pos += span.size_bytes();
	return *this;
}

void Deserializer::ensureSufficientBytesRemaining(size_t bytes)
{
	if (bytes > getBytesRemaining()) {
		throw Exception("Attempt to deserialize out of bounds");
	}
}

size_t Deserializer::getBytesRemaining() const
{
	if (pos > size_t(src.size_bytes())) {
		return 0;
	} else {
		return size_t(src.size_bytes()) - pos;
	}
}
