#pragma once
#include "halley/file/filesystem.h"
#include "halley/text/halleystring.h"
#include <memory>

namespace Halley
{
	class ImportAssetsDatabaseEntry;
	class Metadata;

	enum class AssetType
	{
		UNDEFINED,
		CODEGEN,
		FONT,
		IMAGE,
		MATERIAL,
		SIMPLE_COPY
	};

	class IAssetImporter
	{
	public:
		virtual ~IAssetImporter() {}

		virtual AssetType getType() const = 0;

		virtual String getAssetId(Path file) const = 0;
		virtual std::vector<Path> import(const ImportAssetsDatabaseEntry& asset, Path dstDir) = 0;

		static std::unique_ptr<Metadata> getMetaData(Path path);
	};
}
