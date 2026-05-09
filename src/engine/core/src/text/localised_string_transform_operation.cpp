#include "localised_string_transform_operation.h"

using namespace Halley;

void ILocStrOp::setString(LocalisedString& dst, String value)
{
	dst.string = std::move(value);
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

	auto str = original.getString();
	for (size_t i = 0; i < toks.size(); ++i) {
		str = str.replaceAll("{" + ids[i] + "}", toks[i].getString());
	}
	setString(dst, str);

	/*
	Vector<std::pair<int, size_t>> indices;

	for (int i = 0; i < int(toks.size()); ++i) {
		const auto pos = string.find("{" + Halley::toString(i) + "}");
		if (pos != String::npos) {
			indices.emplace_back(i, pos);
		}
	}

	std::stable_sort(indices.begin(), indices.end(), [] (const auto& a, const auto& b) { return a.second < b.second; });

	auto str = std::string_view(string);

	size_t lastPos = 0;
	ColourStringBuilder builder;
	for (const auto& index: indices) {
		builder.append(str.substr(lastPos, index.second - lastPos));
		builder.append(toks[index.first].getString(), colours[index.first]);
		lastPos = index.second + 3;
	}
	builder.append(str.substr(lastPos));

	auto result = builder.moveResults();
	return { LocalisedString(std::move(result.first), i18n), std::move(result.second) };
	*/
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
