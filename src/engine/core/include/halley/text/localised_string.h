#pragma once

#include "halleystring.h"
#include <map>

#include "halley/graphics/text/text_renderer.h"
#include "halley/maths/colour.h"

namespace Halley {
	class I18N;
	class I18NLanguage;
	class ILocStrOp;

	namespace Detail {
		template<typename T, typename...>
		struct first {
			typedef T type;
		};
	}

	class LocalisedString
	{
		friend class I18N;
		friend class ILocStrOp;

	public:
		LocalisedString();

		LocalisedString(const LocalisedString& other) = default;
		LocalisedString(LocalisedString&& other) noexcept = default;

		LocalisedString& operator=(const LocalisedString& other) = default;
		LocalisedString& operator=(LocalisedString&& other) noexcept = default;
		
		LocalisedString& operator+=(const LocalisedString& str);

		[[nodiscard]] static LocalisedString fromHardcodedString(const char* str);
		[[nodiscard]] static LocalisedString fromHardcodedString(const String& str);
		[[nodiscard]] static LocalisedString fromUserString(const String& str);
		[[nodiscard]] static LocalisedString fromNumber(int number, int base = 10, int width = 1, char fill = '0');
		[[nodiscard]] static LocalisedString fromNumber(float number, const I18NLanguage& code, int precisionDigits = -1, bool fixed = true);

		template<typename... Ts>
		[[nodiscard]] Detail::first<std::enable_if_t<std::is_convertible_v<Ts, const LocalisedString&>, LocalisedString>...>::type replaceTokens(const Ts&... toks)
		{
			auto tmp = Vector<LocalisedString>({ toks... });
			return doReplaceTokens(std::move(tmp));
		}
		
		[[nodiscard]] LocalisedString replaceTokens(gsl::span<const LocalisedString> toks) const;
		[[nodiscard]] std::pair<LocalisedString, Vector<ColourOverride>> replaceTokens(gsl::span<const LocalisedString> toks, gsl::span<const std::optional<Colour4f>> colours) const;
		[[nodiscard]] LocalisedString replaceTokens(const std::map<String, LocalisedString>& tokens) const;
		[[nodiscard]] LocalisedString replaceToken(const String& pattern, const LocalisedString& token) const;

		[[nodiscard]] LocalisedString replaceLanguage(const I18NLanguage& language) const;
		void replaceLanguageInPlace(const I18NLanguage& language);

		const String& getString() const;
		const String& toString() const;

		bool operator==(const LocalisedString& other) const;
		bool operator!=(const LocalisedString& other) const;
		bool operator<(const LocalisedString& other) const;

		LocalisedString operator+(const LocalisedString& other) const;

		bool checkForUpdates();

		const String& getKey() const;
		const I18NLanguage* tryGetLanguage() const;
		const I18NLanguage& getLanguage(const I18N& i18n) const;

	private:
		explicit LocalisedString(String string, const I18N* i18n);
		explicit LocalisedString(const I18N& i18n, String key, String string, int languageIdx);

		[[nodiscard]] LocalisedString doReplaceTokens(Vector<LocalisedString> toks, gsl::span<const std::optional<Colour4f>> cols = {}, Vector<ColourOverride>* outCols = nullptr) const;
		[[nodiscard]] LocalisedString doReplaceTokens(Vector<String> ids, Vector<LocalisedString> toks, gsl::span<const std::optional<Colour4f>> cols = {}, Vector<ColourOverride>* outCols = nullptr) const;

		void applyTransformOperation();

		const I18N* i18n = nullptr;
		String key;
		String string;
		int i18nVersion = 0;
		int languageIdx = 0;

		std::shared_ptr<ILocStrOp> transformOp;
	};

}

