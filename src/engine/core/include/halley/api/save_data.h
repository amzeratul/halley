#pragma once

#include <array>
#include <halley/utils/utils.h>
#include <halley/text/enum_names.h>
#include <limits>

#ifdef max
#undef max
#endif

namespace Halley {
	class String;
	
	enum class SaveDataType {
		SaveRoaming,
		SaveLocal,
		Downloads,
		Cache
	};

	template <>
	struct EnumNames<SaveDataType> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"save",
				"save_local",
				"downloads",
				"cache"
			}};
		}
	};

	class ISaveData {
	public:
		virtual ~ISaveData() = default;

		virtual bool isReady() const = 0;

		virtual Bytes getData(const String& path) = 0;
		virtual void removeData(const String& path) {};
		virtual Vector<String> enumerate(const String& root) = 0;

		virtual void setData(const String& path, const Bytes& data, bool commit = true) = 0;
		virtual void commit() = 0;
		virtual size_t getFreeSpace() { return std::numeric_limits<size_t>::max(); }
	};
}
