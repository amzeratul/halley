#pragma once

#include <array>
#include <halley/utils/utils.h>
#include <halley/text/string_converter.h>

namespace Halley {
	enum class SaveDataType {
		Save,
		Downloads,
		Cache
	};

	template <>
	struct EnumNames<SaveDataType> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"save",
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
		virtual std::vector<String> enumerate(const String& root) = 0;

		virtual void setData(const String& path, const Bytes& data, bool commit = true) = 0;
		virtual void commit() = 0;
	};
}
