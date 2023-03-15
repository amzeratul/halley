#pragma once

#include "halleystring.h"
#include "halley/data_structures/vector.h"
#include <optional>
#include <gsl/span>

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
			Vector<std::pair<uint16_t, uint16_t>> matchPositions;

			bool operator<(const Score& other) const;

			Score advance(int jumpLen, int sectionPos, int newSectionLen) const;
			void makeMatchPositions(const Vector<int>& breadcrumbs);
		};
    	
        class Result {
        public:
        	Result() = default;
            Result(String str, String id, Score score);

            bool operator<(const Result& other) const;

        	const String& getString() const;
        	const String& getId() const;
        	gsl::span<const std::pair<uint16_t, uint16_t>> getMatchPositions() const;

        private:
        	String str;
        	String id;
        	Vector<std::pair<uint16_t, uint16_t>> matchPositions;
        	Score score;
        };

        FuzzyTextMatcher(bool caseSensitive, std::optional<size_t> resultsLimit);
    	
    	void addStrings(Vector<String> strings);
    	void addString(String string, String id = "");
    	void clear();

    	Vector<Result> match(const String& query) const;

    private:
        struct Entry {
	        String string;
            String id;
        };
    	
    	Vector<Entry> strings;
    	bool caseSensitive;
    	std::optional<size_t> resultsLimit;

    	std::optional<Result> match(const String& str, const String& id, const StringUTF32& query) const;
    };
}
