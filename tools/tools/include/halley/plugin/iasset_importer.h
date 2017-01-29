#pragma once
#include <memory>
#include <vector>
#include "halley/text/halleystring.h"
#include "halley/file/path.h"
#include "halley/utils/utils.h"
#include "halley/resources/metadata.h"
#include "halley/resources/resource.h"

namespace Halley
{
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
		using ProgressReporter = std::function<bool(float, const String&)>;
		using AssetCollector = std::function<void(ImportingAsset&&)>;

		virtual ~IAssetImporter() {}

		virtual AssetType getType() const = 0;

		virtual String getAssetId(const Path& file) const;
		virtual std::vector<Path> import(const ImportingAsset& asset, const Path& dstDir, ProgressReporter reporter, AssetCollector collector) = 0;

		void setAssetsSrc(const std::vector<Path>& assetsSrc);

	protected:
		Bytes readAdditionalFile(Path filePath) const;

		std::vector<Path> assetsSrc;
	};
}
