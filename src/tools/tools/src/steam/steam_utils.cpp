#include "steam_utils.h"

#include "halley/os/os.h"

using namespace Halley;

std::optional<Path> SteamUtils::getSteamGameDir(int gameId)
{
	// Referencing https://github.com/NPBruce/valkyrie/issues/1056
		
	// First, read Steam's install dir from registry
	const auto steamPathRaw = OS::get().getRegistryString("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Valve\\Steam\\InstallPath").asString("");
	if (steamPathRaw.isEmpty()) {
		return {};
	}
	const auto steamPath = Path(steamPathRaw);

	// Parse the library manifest
	const auto libraryData = parseVDF(Path::readFileString(steamPath / "steamapps" / "libraryfolders.vdf"));
	if (libraryData.getType() == ConfigNodeType::Undefined) {
		return {};
	}

	// Find the library path
	String libraryPath;
	const auto& foldersNode = libraryData["libraryfolders"];
	if (foldersNode.getType() != ConfigNodeType::Map) {
		return {};
	}
	for (const auto& [k, v]: foldersNode.asMap()) {
		if (v["apps"].hasKey(toString(gameId))) {
			libraryPath = v["path"].asString("");
			break;
		}
	}
	if (libraryPath.isEmpty()) {
		return {};
	}

	// Parse the game manifest
	const auto gameManifestData = parseVDF(Path::readFileString(Path(libraryPath) / "steamapps" / ("appmanifest_" + toString(gameId) + ".acf")));
	if (gameManifestData.getType() == ConfigNodeType::Undefined || !gameManifestData.hasKey("AppState") || !gameManifestData["AppState"].hasKey("installdir")) {
		return {};
	}
	const auto installDir = gameManifestData["AppState"]["installdir"].asString("");
	if (installDir.isEmpty()) {
		return {};
	}

	// Finally, build the final path
	return Path(libraryPath) / "steamapps" / "common" / installDir;
}

ConfigNode SteamUtils::parseVDF(const String& strData)
{
	const auto tokens = tokeniseVDF(strData);
	return makeVDFNode(tokens.const_span());
}

Vector<String> SteamUtils::tokeniseVDF(const String& strData)
{
	Vector<String> tokens;
	std::optional<size_t> tokenStart;

	char prevChar = 0;
	const size_t len = strData.length();
	for (size_t i = 0; i < len; ++i) {
		const char curChar = strData[i];
		const bool isSpace = curChar == ' ' || curChar == '\n' || curChar == '\r' || curChar == '\t';

		if (tokenStart && curChar == '\"' && prevChar != '\\') {
			tokens.push_back(strData.mid(*tokenStart + 1, i - *tokenStart - 1));
			tokenStart = {};
		} else if (!tokenStart && !isSpace) {
			if (curChar == '\"') {
				tokenStart = i;
			} else if (curChar == '{' || curChar == '}') {
				tokens.push_back(String(curChar));
			}
		}

		prevChar = curChar;
	}
	return tokens;
}

ConfigNode SteamUtils::makeVDFNode(gsl::span<const String> tokens)
{
	int idx = 0;
	return makeVDFNode(tokens, idx);
}

ConfigNode SteamUtils::makeVDFNode(gsl::span<const String> tokens, int& idx)
{
	ConfigNode result;
	std::optional<String> key;
	const int n = static_cast<int>(tokens.size());

	while (idx < n) {
		const auto& token = tokens[idx++];

		if (token == "}") {
			// End
			break;
		}

		if (!key) {
			key = token;
		} else {
			if (token == "{") {
				// Recurse
				result[*key] = makeVDFNode(tokens, idx);
			} else {
				// Insert literal
				result[*key] = token.replaceAll("\\\\", "\\");
			}
			key = {};
		}
	}
	return result;
}
