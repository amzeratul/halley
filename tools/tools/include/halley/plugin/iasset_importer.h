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

	class IAssetCollector
	{
	public:
		virtual ~IAssetCollector() {}
		virtual void output(const Path& path, const Bytes& data) = 0;
		virtual void output(const Path& path, Bytes&& data) = 0;
		virtual void output(const Path& path, gsl::span<const gsl::byte> data) = 0;
		virtual void addAdditionalAsset(ImportingAsset&& asset) = 0;
		virtual bool reportProgress(float progress, const String& label = "") = 0;
		virtual Bytes readAdditionalFile(const Path& filePath) const = 0;
		virtual const Path& getDestinationDirectory() = 0;
	};

	class IAssetImporter
	{
	public:
		virtual ~IAssetImporter() {}

		virtual AssetType getType() const = 0;
		virtual void import(const ImportingAsset& asset, IAssetCollector& collector) = 0;

		virtual String getAssetId(const Path& file) const
		{
			String name = file.string();
			if (name.endsWith(".meta")) {
				name = name.substr(0, name.length() - 5);
			}
			return name;
		}
	};
}
