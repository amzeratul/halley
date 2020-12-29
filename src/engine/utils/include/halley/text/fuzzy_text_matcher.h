#pragma once

#include "halleystring.h"
#include <vector>

namespace Halley {
    class FuzzyTextMatcher {
    public:
        class Result {
        public:
        	const String* str = nullptr;
        	std::vector<std::pair<size_t, size_t>> matchPositions;

        	Result() = default;
        	Result(const String& str, std::vector<std::pair<size_t, size_t>> matchPositions)
        		: str(&str)
        		, matchPositions(std::move(matchPositions))
        	{}
        };
    	
    	void addStrings(std::vector<String> strings);
    	void addString(String string);
    	void clear();

    	std::vector<Result> match(const String& query) const;

    private:
    	std::vector<String> strings;

    	std::optional<Result> match(const String& src, const String& query) const;
    };
}
