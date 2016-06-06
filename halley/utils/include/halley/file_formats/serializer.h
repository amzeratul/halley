#pragma once

#include <halley/utils/utils.h>
#include <vector>

namespace Halley {

	class Serializer {
	public:
		Serializer(Bytes& data);

		Serializer& operator<<(bool val);
		Serializer& operator<<(char val);
		Serializer& operator<<(short val);
		Serializer& operator<<(int val);
		Serializer& operator<<(size_t val);
		Serializer& operator<<(float val);
		Serializer& operator<<(long long val);
		Serializer& operator<<(const std::string& str);
		Serializer& operator<<(Bytes& str);

		template <typename T>
		Serializer& operator<<(const std::vector<T>& val)
		{
			size_t sz = val.size();
			*this << short(sz);
			for (size_t i = 0; i < sz; i++) {
				*this << val[i];
			}
			return *this;
		}

		Serializer& operator=(const Serializer&) = delete;

	private:
		Bytes& data;
		void increaseBy(size_t n);
	};

	class Deserializer {
	public:
		Deserializer(Bytes& data);

		void discard(size_t n);
		Deserializer& operator>>(bool& val);
		Deserializer& operator>>(char& val);
		Deserializer& operator>>(short& val);
		Deserializer& operator>>(int& val);
		Deserializer& operator>>(size_t& val);
		Deserializer& operator>>(float& val);
		Deserializer& operator>>(long long& val);
		Deserializer& operator>>(std::string& str);
		Deserializer& operator>>(Bytes& str);

		template <typename T>
		Deserializer& operator>>(std::vector<T>& val)
		{
			short tmp;
			*this >> tmp;
			size_t sz = tmp;
			val.clear();
			val.reserve(sz);
			for (size_t i = 0; i < sz; i++) {
				val.push_back(T());
				*this >> val[i];
			}
			return *this;
		}

		char peekChar() const;

	private:
		Bytes data;
		size_t pos;
	};
}
