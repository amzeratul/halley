#include "halley/text/string_diff.h"
#include "../../../contrib/dtl/dtl.hpp"
#include "halley/utils/algorithm.h"

using namespace Halley;

namespace {
	bool isVisible(const String& str)
	{
		for (auto c: std::string_view(str)) {
			if (c != ' ' && c != '\n' && c != '\t') {
				return true;
			}
		}
		return false;
	}

	String makeVisible(const String& str)
	{
		return str.replaceAll(u8" ", u8"·").replaceAll(u8"\t", u8"\\t").replaceAll(u8"\n", u8"\\n");
	}

	Vector<StringDiffEntry> postProcess(Vector<StringDiffEntry> src)
	{
		// Collapse small common strings to preceding ones
		// e.g. +"Hello" -"Hi" c" " -> +"Hello " -"Hi "
		size_t n = src.size();
		for (size_t i = 0; i < n; ++i) {
			if (i >= 2 && src[i].type == StringDiffType::Common) {
				const auto t0 = src[i - 2].type;
				const auto t1 = src[i - 1].type;
				if ((t0 == StringDiffType::Add && t1 == StringDiffType::Delete) || (t0 == StringDiffType::Delete && t1 == StringDiffType::Add)) {
					if (String::getUTF32Len(src[i].str) <= 2) {
						src[i - 2].str += src[i].str;
						src[i - 1].str += src[i].str;
						src.erase(src.begin() + i);
						--i;
						--n;
					}
				}
			}
		}

		// Merge adds and deletes as long as they don't go over a "common"
		std::optional<size_t> lastAdd;
		std::optional<size_t> lastDel;
		for (size_t i = 0; i < n; ++i) {
			if (src[i].type == StringDiffType::Common) {
				lastAdd = lastDel = std::nullopt;
				continue;
			}

			auto& lastIdx = src[i].type == StringDiffType::Add ? lastAdd : lastDel;
			if (lastIdx) {
				src[*lastIdx].str += src[i].str;
				src.erase(src.begin() + i);
				--i;
				--n;
			} else {
				lastIdx = i;
			}
		}

		// Make whitespace only changes visible
		for (auto& e: src) {
			if (e.type == StringDiffType::Add || e.type == StringDiffType::Delete) {
				if (!isVisible(e.str)) {
					e.str = makeVisible(e.str);
				}
			}
		}

		return src;
	}

	template <typename E, typename S>
	Vector<StringDiffEntry> doMakeDiff(const S& a, const S& b)
	{
		Vector<StringDiffEntry> result;

		auto diff = dtl::Diff<E, S>(a, b);
		diff.compose();

		for (const auto& e: diff.getSes().getSequence()) {
			StringDiffEntry entry;
			switch (e.second.type) {
			case dtl::SES_DELETE:
				entry.type = StringDiffType::Delete;
				break;
			case dtl::SES_COMMON:
				entry.type = StringDiffType::Common;
				break;
			case dtl::SES_ADD:
				entry.type = StringDiffType::Add;
				break;
			default:
				continue;
			}

			if (result.empty() || result.back().type != entry.type) {
				result += entry;
			}
			result.back().str += e.first;
		}

		return postProcess(result);
	}

	bool isSeparator(char c)
	{
		constexpr char delims[] = " ,;.?!:\"[]{}()\n\t";
		return std::find(std::begin(delims), std::end(delims), c) != std::end(delims);
	}

	Vector<String> splitInWords(std::string_view str)
	{
		Vector<String> result;

		size_t curStart = 0;
		for (size_t i = 0; i < str.size(); ++i) {
			const auto c = str[i];
			if (isSeparator(c)) {
				if (i > curStart) {
					result += String(str.substr(curStart, i - curStart));
				}
				result += String(str.substr(i, 1));
				curStart = i + 1;
			}
		}
		if (str.size() > curStart) {
			result += String(str.substr(curStart));
		}

		return result;
	}

	Vector<StringDiffEntry> makeTrivial(std::string_view str, StringDiffType type)
	{
		Vector<StringDiffEntry> result;
		result += StringDiffEntry{ type, str };
		return result;
	}
}

Vector<StringDiffEntry> StringDiff::makeDiff(std::string_view a, std::string_view b)
{
	if (a == b) {
		return makeTrivial(a, StringDiffType::Common);
	} else if (a.empty()) {
		return makeTrivial(b, StringDiffType::Add);
	} else if (b.empty()) {
		return makeTrivial(a, StringDiffType::Delete);
	}

	return doMakeDiff<char, std::string_view>(a, b);
}

Vector<StringDiffEntry> StringDiff::makeWordDiff(std::string_view a, std::string_view b)
{
	if (a == b) {
		return makeTrivial(a, StringDiffType::Common);
	} else if (a.empty()) {
		return makeTrivial(b, StringDiffType::Add);
	} else if (b.empty()) {
		return makeTrivial(a, StringDiffType::Delete);
	}

	Vector<String> wordsA = splitInWords(a);
	Vector<String> wordsB = splitInWords(b);

	return doMakeDiff<String, Vector<String>>(wordsA, wordsB);
}
