#pragma once
#include "halley/text/halleystring.h"
#include <memory>
#include <vector>
#include "halley/file/path.h"
#include "halley/utils/utils.h"

namespace Halley
{
	class Metadata;

	enum class AssetType
	{
		Undefined,
		Codegen,
		SimpleCopy,
		Font,
		Image,
		Material,
		Animation,
		Config,
		Audio,
		Aseprite,
		SpriteSheet
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
		AssetType assetType = AssetType::Undefined;
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
