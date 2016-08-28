#pragma once

#include <halley/utils/utils.h>
#include <vector>
#include <gsl/gsl>
#include "halley/data_structures/flat_map.h"
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"
#include <map>

namespace Halley {
	class String;

	class Serializer {
	public:
		Serializer();
		explicit Serializer(gsl::span<gsl::byte> dst);

		template <typename T>
		static typename std::enable_if<std::is_convertible<T, std::function<void(Serializer&)>>::value, Bytes>::type toBytes(const T& f)
		{
			Serializer dry;
			f(dry);
			Bytes result(dry.getSize());
			Serializer s(gsl::as_writeable_bytes(gsl::span<Halley::Byte>(result)));
			f(s);
			return result;
		}

		template <typename T>
		static typename std::enable_if<!std::is_convertible<T, std::function<void(Serializer&)>>::value, Bytes>::type toBytes(const T& value)
		{
			return toBytes([&value](Serializer& s) { value.serialize(s); });
		}

		size_t getSize() const { return size; }

		Serializer& operator<<(bool val) { return serializePod(val); }
		Serializer& operator<<(char val) { return serializePod(val); }
		Serializer& operator<<(unsigned char val) { return serializePod(val); }
		Serializer& operator<<(short val) { return serializePod(val); }
		Serializer& operator<<(unsigned short val) { return serializePod(val); }
		Serializer& operator<<(int val) { return serializePod(val); }
		Serializer& operator<<(unsigned int val) { return serializePod(val); }
		Serializer& operator<<(long long val) { return serializePod(val); }
		Serializer& operator<<(unsigned long long val) { return serializePod(val); }
		Serializer& operator<<(float val) { return serializePod(val); }
		Serializer& operator<<(double val) { return serializePod(val); }

		Serializer& operator<<(const std::string& str);
		Serializer& operator<<(const String& str);
		Serializer& operator<<(gsl::span<const gsl::byte> span);

		template <typename T>
		Serializer& operator<<(const std::vector<T>& val)
		{
			unsigned int sz = static_cast<unsigned int>(val.size());
			*this << sz;
			for (unsigned int i = 0; i < sz; i++) {
				*this << val[i];
			}
			return *this;
		}

		template <typename T, typename U>
		Serializer& operator<<(const FlatMap<T, U>& val)
		{
			*this << static_cast<unsigned int>(val.size());
			for (auto& kv : val) {
				*this << kv.first << kv.second;
			}
			return *this;
		}

		template <typename T, typename U>
		Serializer& operator<<(const std::map<T, U>& val)
		{
			*this << static_cast<unsigned int>(val.size());
			for (auto& kv : val) {
				*this << kv.first << kv.second;
			}
			return *this;
		}

		template <typename T>
		Serializer& operator<<(const Vector2D<T>& val)
		{
			return *this << val.x << val.y;
		}

		template <typename T>
		Serializer& operator<<(const Rect2D<T>& val)
		{
			return *this << val.getTopLeft() << val.getBottomRight();
		}

		template <typename T>
		Serializer& operator<<(const T& val)
		{
			val.serialize(*this);
			return *this;
		}

	private:
		bool dryRun;
		size_t size = 0;
		gsl::span<gsl::byte> dst;

		template <typename T>
		Serializer& serializePod(T val)
		{
			if (!dryRun) {
				memcpy(dst.data() + size, &val, sizeof(T));
			}
			size += sizeof(T);
			return *this;
		}
	};

	class Deserializer {
	public:
		Deserializer(gsl::span<const gsl::byte> src);

		Deserializer& operator>>(bool& val) { return deserializePod(val); }
		Deserializer& operator>>(char& val) { return deserializePod(val); }
		Deserializer& operator>>(unsigned char& val) { return deserializePod(val); }
		Deserializer& operator>>(short& val) { return deserializePod(val); }
		Deserializer& operator>>(unsigned short& val) { return deserializePod(val); }
		Deserializer& operator>>(int& val) { return deserializePod(val); }
		Deserializer& operator>>(unsigned int& val) { return deserializePod(val); }
		Deserializer& operator>>(long long& val) { return deserializePod(val); }
		Deserializer& operator>>(unsigned long long& val) { return deserializePod(val); }
		Deserializer& operator>>(float& val) { return deserializePod(val); }
		Deserializer& operator>>(double& val) { return deserializePod(val); }

		Deserializer& operator>>(std::string& str);
		Deserializer& operator>>(String& str);
		Deserializer& operator>>(gsl::span<gsl::byte>& span);

		template <typename T>
		Deserializer& operator>>(std::vector<T>& val)
		{
			unsigned int sz;
			*this >> sz;
			val.clear();
			val.reserve(sz);
			for (unsigned int i = 0; i < sz; i++) {
				val.push_back(T());
				*this >> val[i];
			}
			return *this;
		}

		template <typename T, typename U>
		Deserializer& operator>>(FlatMap<T, U>& val)
		{
			unsigned int sz;
			*this >> sz;
			for (unsigned int i = 0; i < sz; i++) {
				T key;
				U value;
				*this >> key;
				*this >> value;
				val[key] = value;
			}
			return *this;
		}

		template <typename T, typename U>
		Deserializer& operator >> (std::map<T, U>& val)
		{
			unsigned int sz;
			*this >> sz;
			for (unsigned int i = 0; i < sz; i++) {
				T key;
				U value;
				*this >> key;
				*this >> value;
				val[key] = value;
			}
			return *this;
		}

		template <typename T>
		Deserializer& operator>>(Vector2D<T>& val)
		{
			*this >> val.x;
			*this >> val.y;
			return *this;
		}

		template <typename T>
		Deserializer& operator>>(Rect2D<T>& val)
		{
			Vector2D<T> p1, p2;
			*this >> p1;
			*this >> p2;
			val = Rect2D<T>(p1, p2);
			return *this;
		}

		template <typename T>
		Deserializer& operator>>(T& val)
		{
			val.deserialize(*this);
			return *this;
		}

	private:
		size_t pos = 0;
		gsl::span<const gsl::byte> src;

		template <typename T>
		Deserializer& deserializePod(T& val)
		{
			memcpy(&val, src.data() + pos, sizeof(T));
			pos += sizeof(T);
			return *this;
		}
	};
}
