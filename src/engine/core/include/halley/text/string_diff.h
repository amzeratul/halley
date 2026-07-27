#pragma once

#include "halleystring.h"
#include "string_converter.h"

namespace Halley {
    enum class StringDiffType {
        Common,
        Delete,
        Add
    };

	template <>
	struct EnumNames<StringDiffType> {
		constexpr auto operator()() const {
			return std::to_array({
				"common",
				"delete",
				"add"
			});
		}
	};

    struct StringDiffEntry {
        StringDiffType type;
        String str;
    };

    class StringDiff {
    public:
        static Vector<StringDiffEntry> makeDiff(std::string_view a, std::string_view b);
        static Vector<StringDiffEntry> makeWordDiff(std::string_view a, std::string_view b);
    };
}
