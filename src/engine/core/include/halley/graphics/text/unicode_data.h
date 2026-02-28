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
            bool forceLineBreakAfter : 1;
            Context context;
        };

        // The actual Unicode rules are a bit crazy, this is a simplified version
        enum class LineBreakClass : uint8_t {
            Unknown,
            Alphabetic,
            Space,
	        Break,
            CarriageReturn,
            LineFeed,
            ClosePunctuation,
            OpenPunctuation,
            Ideographic,
            EmojiBase,
            InfixNumericSeparator,
            Numeric,
            PostfixNumeric,
            PrefixNumeric,
        };

        UnicodeData();

        LineBreakClass getLineBreakClass(char32_t code) const;
        const LineBreakRules& getLineBreakRules(char32_t code, Context context = Context::Text) const;
        const LineBreakRules& getLineBreakRules(LineBreakClass characterClass, Context context = Context::Text) const;
        Context getNewContext(LineBreakClass charClass) const;
        Context getNewContext(LineBreakClass curCharClass, LineBreakClass nextCharClass) const;

        static bool isEastAsianIdeographicCharacter(char32_t c);

    private:
        std::array<LineBreakRules, 14> lineBreakRules;
        HashMap<char32_t, LineBreakClass> lineBreakClasses;
        std::array<LineBreakClass, 256> lineBreakClassesAscii;

        void setClass(char32_t c, LineBreakClass breakClass);
        void loadLineBreakRules();
    };

    class UnicodeLineBreaker {
    public:
        struct Result {
            int priority = 0;
            bool forceBreak = false;
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
