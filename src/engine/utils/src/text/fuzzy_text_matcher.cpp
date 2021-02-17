#include "halley/text/fuzzy_text_matcher.h"
using namespace Halley;

FuzzyTextMatcher::Score FuzzyTextMatcher::Score::advance(int jumpLen, int sectionPos, int newSectionLen) const
{
	Score result = *this;
	if (jumpLen > 0) {
		result.curJumps++;
		result.jumpLength += jumpLen;
	}
	result.sectionPos = std::min(result.sectionPos, sectionPos);
	if (newSectionLen > 0) {
		result.sectionLens += newSectionLen;
		result.sections++;
	}
	return result;
}

void FuzzyTextMatcher::Score::makeMatchPositions(const std::vector<int>& breadcrumbs)
{
	int last = -2;
	for (auto pos: breadcrumbs) {
		if (pos != last + 1) {
			matchPositions.emplace_back(pos, 1);
		} else {
			++matchPositions.back().second;
		}
		last = pos;
	}
}

bool FuzzyTextMatcher::Score::operator<(const Score& other) const
{
	if (curJumps != other.curJumps) {
		return curJumps < other.curJumps;
	}
	if (sections != other.sections) {
		return sections < other.sections;
	}
	if (sectionPos != other.sectionPos) {
		return sectionPos < other.sectionPos;
	}
	if (sectionLens != other.sectionLens) {
		return sectionLens < other.sectionLens;
	}
	return jumpLength < other.jumpLength;
}

FuzzyTextMatcher::Result::Result(String str, Score score)
	: str(std::move(str))
	, matchPositions(std::move(score.matchPositions))
	, score(score)
{
}

bool FuzzyTextMatcher::Result::operator<(const Result& other) const
{
	return score < other.score;
}

const String& FuzzyTextMatcher::Result::getString() const
{
	return str;
}

gsl::span<const std::pair<uint16_t, uint16_t>> FuzzyTextMatcher::Result::getMatchPositions() const
{
	return matchPositions;
}

FuzzyTextMatcher::FuzzyTextMatcher(bool caseSensitive, std::optional<size_t> resultsLimit)
	: caseSensitive(caseSensitive)
	, resultsLimit(resultsLimit)
{
}

void FuzzyTextMatcher::addStrings(std::vector<String> strs)
{
	if (strings.empty()) {
		strings = std::move(strs);
	} else {
		strings.reserve(strings.size() + strs.size());
		for (auto& str: strs) {
			strings.push_back(std::move(str));
		}
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
	
	if (resultsLimit && results.size() > resultsLimit.value()) {
		results.resize(resultsLimit.value());
	}
	
	return results;
}

class FuzzyMatchState {
public:
	std::vector<int> sectionMap;
	std::vector<int> sectionLengths;
	std::vector<int> breadcrumb;
};

static FuzzyTextMatcher::Score findBestScore(const std::vector<std::vector<int16_t>>& indices, int idx, std::optional<int16_t> lastPos, FuzzyTextMatcher::Score score, FuzzyMatchState& state)
{
	if (idx == -1) {
		// Terminate
		score.makeMatchPositions(state.breadcrumb);
		return score;
	}

	std::optional<FuzzyTextMatcher::Score> bestScore;
	for (size_t i = 0; i < indices[idx].size(); ++i) {
		const auto pos = indices[idx][i];
		state.breadcrumb[idx] = pos;
		
		if (lastPos && pos >= lastPos.value()) {
			break;
		}

		const int curSection = state.sectionMap[pos];
		const bool newSection = !lastPos || state.sectionMap[lastPos.value()] != curSection;
		const int jumps = (lastPos ? (lastPos.value() - pos - 1) : 0);
		const int sectionPos = static_cast<int>(state.sectionLengths.size()) - curSection - 1;
		const auto updatedScore = score.advance(jumps, sectionPos, newSection ? state.sectionLengths[curSection] : 0);
		
		const auto score = findBestScore(indices, idx - 1, pos, updatedScore, state);
		if (bestScore) {
			bestScore = std::min(score, bestScore.value());
		} else {
			bestScore = score;
		}
	}

	return bestScore.value();
}

static FuzzyTextMatcher::Score findBestScore(const std::vector<std::vector<int16_t>>& indices, FuzzyMatchState state)
{
	return findBestScore(indices, static_cast<int>(indices.size()) - 1, {}, FuzzyTextMatcher::Score(), state);
}

static FuzzyMatchState makeState(const StringUTF32& str, const StringUTF32& query)
{
	FuzzyMatchState state;
	state.sectionMap.reserve(str.size());

	bool nextIsNewSection = true;
	for (const char32_t chr: str) {
		bool newSection = nextIsNewSection;
		nextIsNewSection = false;
		if (chr == '/' || chr == '.' || chr == '\\') {
			newSection = true;
			nextIsNewSection = true;
		}

		if (newSection) {
			state.sectionLengths.push_back(1);
		} else {
			++state.sectionLengths.back();
		}
		state.sectionMap.push_back(static_cast<int>(state.sectionLengths.size()) - 1);
	}

	state.breadcrumb.resize(query.size());

	return state;
}

std::optional<FuzzyTextMatcher::Result> FuzzyTextMatcher::match(const String& origStr, const StringUTF32& query) const
{
	if (origStr.isEmpty() || query.empty()) {
		return {};
	}
	StringUTF32 str = caseSensitive ? origStr.getUTF32() : origStr.asciiLower().getUTF32();
	
	std::vector<std::vector<int16_t>> indices;
	
	size_t maxDepth = 1; // maxDepth is 1 more than the number of characters found so far, and represents how far into the query we can look

	for (size_t i = 0; i < str.size(); ++i) {
		// For each character in the string
		const char32_t strChr = str[i];

		// Check up to maxDepth
		for (size_t j = 0; j < maxDepth; ++j) {
			const char32_t queryChr = query[j];

			if (strChr == queryChr) {
				if (j == indices.size()) {
					// Expand indices
					indices.emplace_back();
				}

				// Insert a copy
				indices[j].push_back(static_cast<int16_t>(i));
			}
		}
		maxDepth = std::min(indices.size() + 1, query.size());

		if (query.size() - indices.size() > str.size() - i - 1) {
			// Can't find anything else
			break;
		}
	}

	if (indices.size() == query.size()) {
		// Found something
		return Result(origStr, findBestScore(indices, makeState(str, query)));
	} else {
		// Didn't find anything
		return {};
	}
}
