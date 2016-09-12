#pragma once
#include "halley/file/filesystem.h"
#include "halley/text/halleystring.h"
#include <memory>
#include <vector>

namespace Halley
{
	class Metadata;

	enum class AssetType
	{
		UNDEFINED,
		CODEGEN,
		FONT,
		IMAGE,
		MATERIAL,
		ANIMATION,
		CONFIG,
		SIMPLE_COPY
	};

	class ImportingAssetFile
	{
	public:
		Path name;
		Bytes data;

		ImportingAssetFile() {}
		ImportingAssetFile(const Path& path, Bytes&& data)
			: name(path)
			, data(data)
		{}
	};

	class ImportingAsset
	{
	public:
		String assetId;
		std::vector<ImportingAssetFile> inputFiles;
		std::unique_ptr<Metadata> metadata;
		AssetType assetType = AssetType::UNDEFINED;
	};

	class IAssetImporter
	{
	public:
		using ProgressReporter = std::function<bool(float, String)>;
		using AssetCollector = std::function<void(ImportingAsset&&)>;

		virtual ~IAssetImporter() {}

		virtual AssetType getType() const = 0;

		virtual String getAssetId(Path file) const;
		virtual std::vector<Path> import(const ImportingAsset& asset, Path dstDir, ProgressReporter reporter, AssetCollector collector) = 0;
	};
}
