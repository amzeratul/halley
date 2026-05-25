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
		dst = original;
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

bool LocStrOpReplaceTokens::isEquivalentTo(const ILocStrOp& otherRaw) const
{
	if (auto* other = dynamic_cast<const LocStrOpReplaceTokens*>(&otherRaw)) {
		return isEquivalentToReplaceTokens(*other);
	} else {
		return false;
	}
}

bool LocStrOpReplaceTokens::isEquivalentToReplaceTokens(const LocStrOpReplaceTokens& other) const
{
	if (toks.size() != other.toks.size()) {
		return false;
	}
	if (!original.isSameKeyAndTransform(other.original)) {
		return false;
	}

	const size_t n = toks.size();
	for (size_t i = 0; i < n; ++i) {
		if (ids[i] != other.ids[i]) {
			return false;
		}
		if (!toks[i].isSameKeyAndTransform(other.toks[i])) {
			return false;
		}
	}

	return true;
}



LocStrOpSubstr::LocStrOpSubstr(LocalisedString original, size_t offset, size_t count)
	: original(std::move(original))
	, offset(offset)
	, count(count)
{
}

void LocStrOpSubstr::eval(LocalisedString& dst)
{
	auto tokenInfo = original.getTokenInfo();
	auto result32 = original.getString().getUTF32().substr(offset, count);

	for (auto& token: tokenInfo) {
		if (token.pos >= offset) {
			token.pos -= static_cast<uint32_t>(offset);
		} else {
			token.pos = 0;
		}
	}

	setString(dst, String(result32), tokenInfo);
}

bool LocStrOpSubstr::checkForUpdates()
{
	return original.checkForUpdates();
}

void LocStrOpSubstr::setLanguage(const I18NLanguage& language)
{
	original.replaceLanguageInPlace(language);
}

std::shared_ptr<ILocStrOp> LocStrOpSubstr::clone() const
{
	return std::make_shared<LocStrOpSubstr>(original, offset, count);
}

bool LocStrOpSubstr::isEquivalentTo(const ILocStrOp& otherRaw) const
{
	if (auto* other = dynamic_cast<const LocStrOpSubstr*>(&otherRaw)) {
		return isEquivalentToSubstr(*other);
	} else {
		return false;
	}
}

bool LocStrOpSubstr::isEquivalentToSubstr(const LocStrOpSubstr& other) const
{
	return offset == other.offset && count == other.count && original.isSameKeyAndTransform(other.original);
}
