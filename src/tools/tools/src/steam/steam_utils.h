#pragma once

#include "halley/data_structures/config_node.h"
#include "halley/file/path.h"

namespace Halley {
	class SteamUtils {
	public:
		static std::optional<Path> getSteamGameDir(int gameId);

		static ConfigNode parseVDF(const String& strData);

	private:
		static Vector<String> tokeniseVDF(const String& strData);
		static ConfigNode makeVDFNode(gsl::span<const String> tokens);
		static ConfigNode makeVDFNode(gsl::span<const String> tokens, int& idx);
	};
}
