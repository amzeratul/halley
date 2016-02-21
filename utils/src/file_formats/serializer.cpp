#include "serializer.h"
using namespace Halley;

Serializer::Serializer(Bytes& _data)
	: data(_data)
{
}

Serializer& Serializer::operator<<(const std::string& str)
{
	size_t sz = str.size();
	*this << sz;
	size_t pos = data.size();
	increaseBy(sz);
	memcpy_s(data.data() + pos, sz, str.data(), sz);
	return *this;
}

Serializer& Serializer::operator<<(__int64 val)
{
	size_t pos = data.size();
	increaseBy(8);
	*(reinterpret_cast<__int64*>(data.data() + pos)) = val;
	return *this;
}

Serializer& Serializer::operator<<(short val)
{
	size_t pos = data.size();
	increaseBy(2);
	*(reinterpret_cast<short*>(data.data() + pos)) = val;
	return *this;
}

Serializer& Serializer::operator<<(int val)
{
	size_t pos = data.size();
	increaseBy(4);
	*(reinterpret_cast<int*>(data.data() + pos)) = val;
	return *this;
}

Serializer& Serializer::operator<<(size_t val)
{
	size_t pos = data.size();
	increaseBy(4);
	*(reinterpret_cast<size_t*>(data.data() + pos)) = val;
	return *this;
}

Serializer& Serializer::operator<<(char val)
{
	size_t pos = data.size();
	increaseBy(1);
	*(reinterpret_cast<char*>(data.data() + pos)) = val;
	return *this;
}

Serializer& Serializer::operator<<(bool val)
{
	size_t pos = data.size();
	increaseBy(1);
	*(reinterpret_cast<bool*>(data.data() + pos)) = val;
	return *this;
}

Serializer& Serializer::operator<<(Bytes& val)
{
	short sz = short(val.size());
	*this << sz;

	size_t pos = data.size();
	increaseBy(sz);
	memcpy_s(data.data() + pos, sz, val.data(), sz);

	return *this;
}

Serializer& Serializer::operator<<(float val)
{
	size_t pos = data.size();
	increaseBy(4);
	*(reinterpret_cast<float*>(data.data() + pos)) = val;
	return *this;
}

void Serializer::increaseBy(size_t n)
{
	size_t target = data.size() + n;
	if (target > data.capacity()) {
		data.reserve(std::max(target, data.capacity() * 2));
	}
	data.resize(target);
}

Deserializer::Deserializer(Bytes& _data)
	: data(_data)
	, pos(0)
{

}

Deserializer& Deserializer::operator>>(std::string& str)
{
	size_t sz;
	*this >> sz;
	str = std::string(reinterpret_cast<const char*>(data.data() + pos), sz);
	pos += sz;
	return *this;
}

Deserializer& Deserializer::operator>>(short& val)
{
	val = *(reinterpret_cast<short*>(data.data() + pos));
	pos += 2;
	return *this;
}

Deserializer& Deserializer::operator>>(int& val)
{
	val = *(reinterpret_cast<int*>(data.data() + pos));
	pos += 4;
	return *this;
}

Deserializer& Deserializer::operator>>(size_t& val)
{
	val = *(reinterpret_cast<size_t*>(data.data() + pos));
	pos += 4;
	return *this;
}

Deserializer& Deserializer::operator>>(__int64& val)
{
	val = *(reinterpret_cast<__int64*>(data.data() + pos));
	pos += 8;
	return *this;
}

Deserializer& Deserializer::operator>>(bool& val)
{
	val = *(reinterpret_cast<bool*>(data.data() + pos));
	pos += 1;
	return *this;
}

Deserializer& Deserializer::operator>>(char& val)
{
	val = *(reinterpret_cast<char*>(data.data() + pos));
	pos += 1;
	return *this;
}

Deserializer& Deserializer::operator>>(Bytes& val)
{
	short sz;
	*this >> sz;

	val.resize(sz);
	memcpy_s(val.data(), sz, data.data() + pos, sz);
	pos += sz;

	return *this;
}

Deserializer& Deserializer::operator>>(float& val)
{
	val = *(reinterpret_cast<float*>(data.data() + pos));
	pos += 4;
	return *this;
}

void Deserializer::discard(size_t n)
{
	pos += n;
}

char Halley::Deserializer::peekChar() const
{
	return *(reinterpret_cast<const char*>(data.data() + pos));
}

