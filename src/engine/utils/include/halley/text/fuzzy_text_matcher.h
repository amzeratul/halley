#pragma once

#include "halleystring.h"
#include <vector>

namespace Halley {
    class FuzzyTextMatcher {
    public:
        class Result {
        public:
        	String str = nullptr;
        	std::vector<std::pair<size_t, size_t>> matchPositions;
        	int score = 0;

        	Result() = default;
        	Result(String str, std::vector<std::pair<size_t, size_t>> matchPositions, int score)
        		: str(std::move(str))
        		, matchPositions(std::move(matchPositions))
        		, score(score)
        	{}

        	bool operator<(const Result& other) const
        	{
        		return score < other.score;
        	}
        };

        explicit FuzzyTextMatcher(bool caseSensitive);
    	
    	void addStrings(const std::vector<String>& strings);
    	void addString(const String& string);
    	void clear();

    	std::vector<Result> match(const String& query) const;

    private:
    	std::vector<StringUTF32> strings;
    	bool caseSensitive;

    	std::optional<Result> match(const StringUTF32& str, const StringUTF32& query) const;
    };
}
