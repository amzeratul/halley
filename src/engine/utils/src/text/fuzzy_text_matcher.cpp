#include "halley/text/fuzzy_text_matcher.h"
using namespace Halley;

FuzzyTextMatcher::FuzzyTextMatcher(bool caseSensitive)
	: caseSensitive(caseSensitive)
{
}

void FuzzyTextMatcher::addStrings(std::vector<String> strs)
{
	strings.reserve(strings.size() + strs.size());
	for (auto& str: strs) {
		strings.push_back(caseSensitive ? std::move(str) : str.asciiLower());
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

std::vector<FuzzyTextMatcher::Result> FuzzyTextMatcher::match(String rawQuery) const
{
	const String query = caseSensitive ? std::move(rawQuery) : rawQuery.asciiLower();
	
	std::vector<Result> results;

	for (const auto& str: strings) {
		auto result = match(str, query);
		if (result) {
			results.push_back(std::move(result.value()));
		}
	}

	std::sort(results.begin(), results.end());
	
	return results;
}

std::optional<FuzzyTextMatcher::Result> FuzzyTextMatcher::match(const String& str, const String& query) const
{
	// TODO
	const size_t pos = str.cppStr().rfind(query.cppStr());
	if (pos != std::string::npos) {
		return Result(str, {}, static_cast<int>(str.size() - pos));
	}
	return {};
}
