#include "halley/text/fuzzy_text_matcher.h"
using namespace Halley;

void FuzzyTextMatcher::addStrings(std::vector<String> strs)
{
	strings.reserve(strings.size() + strs.size());
	for (auto& str: strs) {
		strings.push_back(std::move(str));
	}
}

void FuzzyTextMatcher::addString(String string)
{
	strings.push_back(std::move(string));
}

void FuzzyTextMatcher::clear()
{
	strings.clear();
}

std::vector<FuzzyTextMatcher::Result> FuzzyTextMatcher::match(const String& query) const
{
	std::vector<Result> results;

	for (const auto& str: strings) {
		auto result = match(str, query);
		if (result) {
			results.push_back(std::move(result.value()));
		}
	}
	
	return results;
}

std::optional<FuzzyTextMatcher::Result> FuzzyTextMatcher::match(const String& src, const String& query) const
{
	// TODO
	if (src.asciiLower().contains(query)) {
		return Result(src, {});
	}
	return {};
}
