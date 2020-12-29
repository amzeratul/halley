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
        	int score = 0;

        	Result() = default;
        	Result(const String& str, std::vector<std::pair<size_t, size_t>> matchPositions, int score)
        		: str(&str)
        		, matchPositions(std::move(matchPositions))
        		, score(score)
        	{}

        	bool operator<(const Result& other) const
        	{
        		return score < other.score;
        	}
        };

        explicit FuzzyTextMatcher(bool caseSensitive);
    	
    	void addStrings(std::vector<String> strings);
    	void addString(String string);
    	void clear();

    	std::vector<Result> match(String query) const;

    private:
    	std::vector<String> strings;
    	bool caseSensitive;

    	std::optional<Result> match(const String& str, const String& query) const;
    };
}
