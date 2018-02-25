#pragma once

#include "halleystring.h"
#include <map>

namespace Halley {
	class ConfigFile;

	class I18N {
	public:
		I18N();

		void setCurrentLanguage(const String& code);
		void setDefaultLanguage(const String& code);
		void loadLanguage(const String& code, const ConfigFile& config);

		String get(const String& key) const;
		const String& getCurrentLanguage() const;

	private:
		String currentLanguage;
		String defaultLanguage;
		std::map<String, std::map<String, String>> strings;
	};
}
