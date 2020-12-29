#pragma once

#include "halleystring.h"
#include <vector>

namespace Halley {
    class FuzzyTextMatcher {
    public:
		class Score {
		public:
			int curJumps = 0;
			int jumpLength = 0;
			int sections = 0;
			int sectionLens = 0;
			int sectionPos = 0xFFFF;

			Score advance(int jumpLen, int sectionPos, int newSectionLen) const;
			bool operator<(const Score& other) const;
		};
    	
        class Result {
        public:
        	String str = nullptr;
        	std::vector<std::pair<size_t, size_t>> matchPositions;
        	Score score;

        	Result() = default;
            Result(String str, std::vector<std::pair<size_t, size_t>> matchPositions, Score score);

            bool operator<(const Result& other) const;
        };

        FuzzyTextMatcher(bool caseSensitive, std::optional<size_t> resultsLimit);
    	
    	void addStrings(const std::vector<String>& strings);
    	void addString(const String& string);
    	void clear();

    	std::vector<Result> match(const String& query) const;

    private:
    	std::vector<StringUTF32> strings;
    	bool caseSensitive;
    	std::optional<size_t> resultsLimit;

    	std::optional<Result> match(const StringUTF32& str, const StringUTF32& query) const;
    };
}
