#pragma once

#include <halley/utils/utils.h>
#include <vector>
#include <gsl/gsl>
#include "halley/data_structures/flat_map.h"
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"
#include "halley/file/path.h"
#include <map>
#include <unordered_map>
#include <cstdint>
#include <utility>
#include <boost/optional.hpp>
#include "halley/maths/vector4.h"

namespace Halley {
	class String;

	class Serializer {
	public:
		Serializer();
		explicit Serializer(gsl::span<gsl::byte> dst);

		template <typename T, typename std::enable_if<std::is_convertible<T, std::function<void(Serializer&)>>::value, int>::type = 0>
		static Bytes toBytes(const T& f)
		{
			Serializer dry;
			f(dry);
			Bytes result(dry.getSize());
			Serializer s(gsl::as_writeable_bytes(gsl::span<Halley::Byte>(result)));
			f(s);
			return result;
		}

		template <typename T, typename std::enable_if<!std::is_convertible<T, std::function<void(Serializer&)>>::value, int>::type = 0>
		static Bytes toBytes(const T& value)
		{
			return toBytes([&value](Serializer& s) { s << value; });
		}

		size_t getSize() const { return size; }

		Serializer& operator<<(bool val) { return serializePod(val); }
		Serializer& operator<<(int8_t val) { return serializePod(val); }
		Serializer& operator<<(uint8_t val) { return serializePod(val); }
		Serializer& operator<<(int16_t val) { return serializePod(val); }
		Serializer& operator<<(uint16_t val) { return serializePod(val); }
		Serializer& operator<<(int32_t val) { return serializePod(val); }
		Serializer& operator<<(uint32_t val) { return serializePod(val); }
		Serializer& operator<<(int64_t val) { return serializePod(val); }
		Serializer& operator<<(uint64_t val) { return serializePod(val); }
		Serializer& operator<<(float val) { return serializePod(val); }
		Serializer& operator<<(double val) { return serializePod(val); }

		Serializer& operator<<(const std::string& str);
		Serializer& operator<<(const String& str);
		Serializer& operator<<(const Path& path);
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

		template <typename T, typename U>
		Serializer& operator<<(const std::unordered_map<T, U>& val)
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
		Serializer& operator<<(const Vector4D<T>& val)
		{
			return *this << val.x << val.y << val.z << val.w;
		}

		template <typename T>
		Serializer& operator<<(const Rect2D<T>& val)
		{
			return *this << val.getTopLeft() << val.getBottomRight();
		}

		template <typename T, typename U>
		Serializer& operator<<(const std::pair<T, U>& p)
		{
			return *this << p.first << p.second;
		}
		
		template <typename T>
		Serializer& operator<<(const boost::optional<T>& p)
		{
			if (p) {
				return *this << true << p.get();
			} else {
				return *this << false;
			}
		}

		template <typename T, std::enable_if_t<std::is_enum<T>::value == true, int> = 0>
		Serializer& operator<<(const T& val)
		{
			using B = typename std::underlying_type<T>::type;
			return *this << B(val);
		}

		template <typename T, std::enable_if_t<std::is_enum<T>::value == false, int> = 0>
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
		explicit Deserializer(const Bytes& src);
		
		template <typename T>
		static T fromBytes(const Bytes& src)
		{
			T result;
			Deserializer s(src);
			s >> result;
			return result;
		}

		template <typename T>
		static T fromBytes(gsl::span<const gsl::byte> src)
		{
			T result;
			Deserializer s(src);
			s >> result;
			return result;
		}

		template <typename T>
		static void fromBytes(T& target, const Bytes& src)
		{
			Deserializer s(src);
			s >> target;
		}

		template <typename T>
		static void fromBytes(T& target, gsl::span<const gsl::byte> src)
		{
			Deserializer s(src);
			s >> target;
		}

		Deserializer& operator>>(bool& val) { return deserializePod(val); }
		Deserializer& operator>>(int8_t& val) { return deserializePod(val); }
		Deserializer& operator>>(uint8_t& val) { return deserializePod(val); }
		Deserializer& operator>>(int16_t& val) { return deserializePod(val); }
		Deserializer& operator>>(uint16_t& val) { return deserializePod(val); }
		Deserializer& operator>>(int32_t& val) { return deserializePod(val); }
		Deserializer& operator>>(uint32_t& val) { return deserializePod(val); }
		Deserializer& operator>>(int64_t& val) { return deserializePod(val); }
		Deserializer& operator>>(uint64_t& val) { return deserializePod(val); }
		Deserializer& operator>>(float& val) { return deserializePod(val); }
		Deserializer& operator>>(double& val) { return deserializePod(val); }

		Deserializer& operator>>(std::string& str);
		Deserializer& operator>>(String& str);
		Deserializer& operator>>(Path& p);
		Deserializer& operator>>(gsl::span<gsl::byte>& span);

		template <typename T>
		Deserializer& operator>>(std::vector<T>& val)
		{
			unsigned int sz;
			*this >> sz;
			ensureSufficientBytesRemaining(sz); // Expect at least one byte per vector entry

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
			ensureSufficientBytesRemaining(sz); // Expect at least one byte per map entry

			std::vector<std::pair<T, U>> tmpData(sz);
			for (unsigned int i = 0; i < sz; i++) {
				*this >> tmpData[i].first >> tmpData[i].second;
			}
			val = FlatMap<T, U>(boost::container::ordered_unique_range_t(), tmpData.begin(), tmpData.end());
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
				*this >> key >> value;
				val[key] = std::move(value);
			}
			return *this;
		}

		template <typename T, typename U>
		Deserializer& operator >> (std::unordered_map<T, U>& val)
		{
			unsigned int sz;
			*this >> sz;
			for (unsigned int i = 0; i < sz; i++) {
				T key;
				U value;
				*this >> key >> value;
				val[key] = std::move(value);
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
		Deserializer& operator>>(Vector4D<T>& val)
		{
			*this >> val.x;
			*this >> val.y;
			*this >> val.z;
			*this >> val.w;
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

		template <typename T, typename U>
		Deserializer& operator>>(std::pair<T, U>& p)
		{
			return *this >> p.first >> p.second;
		}

		template <typename T>
		Deserializer& operator>>(boost::optional<T>& p)
		{
			bool present;
			*this >> present;
			if (present) {
				T tmp;
				*this >> tmp;
				p = tmp;
			} else {
				p = boost::optional<T>();
			}
			return *this;
		}

		template <typename T, std::enable_if_t<std::is_enum<T>::value == true, int> = 0>
		Deserializer& operator>>(T& val)
		{
			typename std::underlying_type<T>::type tmp;
			*this >> tmp;
			val = T(tmp);
			return *this;
		}

		template <typename T, std::enable_if_t<std::is_enum<T>::value == false, int> = 0>
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
			ensureSufficientBytesRemaining(sizeof(T));
			memcpy(&val, src.data() + pos, sizeof(T));
			pos += sizeof(T);
			return *this;
		}

		void ensureSufficientBytesRemaining(size_t bytes);
		size_t getBytesRemaining() const;
	};
}
