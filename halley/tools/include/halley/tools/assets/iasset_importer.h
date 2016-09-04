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
		using ProgressReporter = std::function<bool(float, String)>;

		virtual ~IAssetImporter() {}

		virtual AssetType getType() const = 0;

		virtual String getAssetId(Path file) const;
		virtual std::vector<Path> import(const ImportAssetsDatabaseEntry& asset, Path dstDir, ProgressReporter reporter) = 0;

		static Path getMainFile(const ImportAssetsDatabaseEntry& asset);
		static std::unique_ptr<Metadata> getMetaData(const ImportAssetsDatabaseEntry& asset);
	};
}
