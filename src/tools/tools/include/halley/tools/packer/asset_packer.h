#pragma once

namespace Halley {
	class AssetPackManifest;
	class Path;

	class AssetPacker {
	public:
		static void pack(const AssetPackManifest& manifest, const Path& assetsDir, const Path& dst);
	};
}
