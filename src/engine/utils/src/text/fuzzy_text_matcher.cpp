#include "halley/text/fuzzy_text_matcher.h"
using namespace Halley;

FuzzyTextMatcher::FuzzyTextMatcher(bool caseSensitive)
	: caseSensitive(caseSensitive)
{
}

void FuzzyTextMatcher::addStrings(const std::vector<String>& strs)
{
	strings.reserve(strings.size() + strs.size());
	for (auto& str: strs) {
		strings.push_back(caseSensitive ? str.getUTF32() : str.asciiLower().getUTF32());
	}
}

void FuzzyTextMatcher::addString(const String& string)
{
	strings.push_back(string.getUTF32());
}

void FuzzyTextMatcher::clear()
{
	strings.clear();
}

std::vector<FuzzyTextMatcher::Result> FuzzyTextMatcher::match(const String& rawQuery) const
{
	const StringUTF32 query = caseSensitive ? rawQuery.getUTF32() : rawQuery.asciiLower().getUTF32();
	
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

static int findBestScore(const std::vector<std::vector<int16_t>>& indices, int idx, std::optional<int16_t> lastPos, int curJumps)
{
	if (idx == -1) {
		// Terminate
		return curJumps;
	}
	
	int bestScore = std::numeric_limits<int>::max();
	for (auto pos: indices[idx]) {
		if (!lastPos || pos < lastPos.value()) {
			const int jumps = curJumps + (lastPos ? (lastPos.value() - pos - 1) : 0);
			const int score = findBestScore(indices, idx - 1, pos, jumps);
			bestScore = std::min(score, bestScore);
		}
	}

	return bestScore;
}

static int findBestScore(const std::vector<std::vector<int16_t>>& indices)
{
	return findBestScore(indices, static_cast<int>(indices.size()) - 1, {}, 0);
}

std::optional<FuzzyTextMatcher::Result> FuzzyTextMatcher::match(const StringUTF32& str, const StringUTF32& query) const
{
	if (str.empty() || query.empty()) {
		return {};
	}
	
	std::vector<std::vector<int16_t>> indices;
	
	size_t maxDepth = 1; // maxDepth is 1 more than the number of characters found so far, and represents how far into the query we can look

	for (size_t i = 0; i < str.size(); ++i) {
		// For each character in the string
		const char32_t strChr = str[i];

		// Check up to maxDepth
		for (size_t j = 0; j < maxDepth; ++j) {
			const char32_t queryChr = query[j];

			if (strChr == queryChr) {
				if (j + 1 == maxDepth) {
					// Expand indices
					indices.emplace_back();
					maxDepth = std::min(maxDepth + 1, query.size());
				}

				// Insert a copy
				indices[j].push_back(static_cast<int16_t>(i));
			}
		}

		if (query.size() - indices.size() > str.size() - i - 1) {
			// Not found
			return {};
		}
	}

	// Found something
	return Result(String(str), {}, findBestScore(indices));
}
