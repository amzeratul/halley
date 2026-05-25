#include "localised_string_transform_operation.h"

#include "halley/utils/algorithm.h"

using namespace Halley;

void ILocStrOp::setString(LocalisedString& dst, String value, Vector<LocalisedString::TokenInfo> tokenInfos)
{
	dst.string = std::move(value);
	dst.tokenInfo = std::move(tokenInfos);
}

LocStrOpReplaceTokens::LocStrOpReplaceTokens(LocalisedString original, Vector<String> ids, Vector<LocalisedString> toks)
	: original(std::move(original))
	, ids(std::move(ids))
	, toks(std::move(toks))
{
	HalleyAssertRelease(this->ids.size() == this->toks.size());
}

void LocStrOpReplaceTokens::eval(LocalisedString& dst)
{
	if (toks.empty()) {
		return;
	}

	Vector<LocalisedString::TokenInfo> tokenInfo;

	String result;
	auto strRemaining = std::string_view(original.getString());
	while (!strRemaining.empty()) {
		auto bracketStart = strRemaining.find('{');
		auto bracketEnd = strRemaining.find('}', bracketStart);
		if (bracketEnd == std::string::npos) {
			bracketStart = std::string::npos;
		}

		// Everything before {
		result += strRemaining.substr(0, bracketStart);

		if (bracketStart == std::string::npos) {
			// Done
			break;
		} else {
			// Everything inside {}
			const auto tokenId = strRemaining.substr(bracketStart + 1, bracketEnd - bracketStart - 1);

			if (auto idx = std_ex::find_index(ids, tokenId)) {
				const auto& token = toks[*idx];
				const auto p0 = String::getUTF32Len(result);
				const auto len = String::getUTF32Len(token.getString());
				tokenInfo += LocalisedString::TokenInfo{ static_cast<uint32_t>(p0), static_cast<uint16_t>(len), static_cast<uint16_t>(*idx) };
				result += token.getString();
			} else {
				// Token not found, write out original string
				result += "{";
				result += tokenId;
				result += "}";
			}
		}

		// Update remaining
		strRemaining = strRemaining.substr(bracketEnd + 1);
	}
	setString(dst, result, tokenInfo);
}

bool LocStrOpReplaceTokens::checkForUpdates()
{
	bool changed = original.checkForUpdates();
	for (auto& tok: toks) {
		changed = tok.checkForUpdates() || changed;
	}
	return changed;
}

void LocStrOpReplaceTokens::setLanguage(const I18NLanguage& language)
{
	original.replaceLanguageInPlace(language);
	for (auto& tok: toks) {
		tok.replaceLanguageInPlace(language);
	}
}

std::shared_ptr<ILocStrOp> LocStrOpReplaceTokens::clone() const
{
	return std::make_shared<LocStrOpReplaceTokens>(original, ids, toks);
}
