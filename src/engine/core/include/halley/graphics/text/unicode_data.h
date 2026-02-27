#pragma once

#include "halley/data_structures/hash_map.h"

namespace Halley {
    // See https://www.unicode.org/reports/tr14/#QU
    class UnicodeData {
    public:
        enum class Context : uint8_t {
            Undefined,
	        Text,
            Numeric
        };

    	struct LineBreakRules {
            bool lineBreakOpportunityBefore : 1;
            bool lineBreakOpportunityAfter : 1;
            bool prohibitLineBreakBefore: 1;
            bool prohibitLineBreakAfter: 1;
            bool isSpace: 1;
            bool consumeMoreSpace : 1;
            Context context;
        };

        // The actual Unicode rules are a bit crazy, this is a simplified version
        enum class LineBreakClass : uint8_t {
            Unknown,
            AL, // Alphabetic
            SP, // Space
	        BK, // Break
            CL, // Close punctuation
            OP, // Open punctuation
            ID, // Ideographic
            EB, // Emoji base
            IS, // Infix numeric separator
            NU, // Numeric
            PO, // Postfix Numeric
            PR, // Prefix Numeric
        };

        UnicodeData();

        LineBreakClass getLineBreakClass(char32_t code) const;
        const LineBreakRules& getLineBreakRules(char32_t code, Context context = Context::Text) const;
        const LineBreakRules& getLineBreakRules(LineBreakClass characterClass, Context context = Context::Text) const;
        Context getNewContext(LineBreakClass charClass) const;
        Context getNewContext(LineBreakClass curCharClass, LineBreakClass nextCharClass) const;

        static bool isEastAsianIdeographicCharacter(char32_t c);

    private:
        HashMap<char32_t, LineBreakClass> lineBreakClasses;
        std::array<LineBreakRules, 12> lineBreakRules;

        void loadLineBreakRules();
    };

    class UnicodeLineBreaker {
    public:
        struct Result {
            int priority = 0;
            bool consumeSpace = false;
            bool hasMoreSpaces = false;
        };

        UnicodeLineBreaker(const UnicodeData& data);
        Result feedCharacter(char32_t curCharacter, char32_t nextCharacter);

    private:
        const UnicodeData& data;
        UnicodeData::Context context = UnicodeData::Context::Text;
    };
}
