#pragma once
#include <memory>
#include <vector>
#include "halley/text/halleystring.h"
#include "halley/file/path.h"
#include "halley/utils/utils.h"
#include "halley/resources/metadata.h"
#include "halley/resources/resource.h"
#include "halley/data_structures/maybe.h"
#include "halley/file/byte_serializer.h"

namespace Halley
{
	class ImportingAssetFile
	{
	public:
		Path name;
		Bytes data;
		Metadata metadata;

		ImportingAssetFile() {}
		ImportingAssetFile(const Path& path, Bytes&& data, const Metadata& meta)
			: name(path)
			, data(data)
			, metadata(meta)
		{}
	};

	class ImportingAsset
	{
	public:
		String assetId;
		std::vector<ImportingAssetFile> inputFiles;
		ImportAssetType assetType = ImportAssetType::Undefined;
	};

	class AssetResource
	{
	public:
		String name;
		AssetType type;
		String filepath;
		Metadata metadata;

		void serialize(Serializer& s) const
		{
			s << name;
			s << type;
			s << filepath;
			s << metadata;
		}
		
		void deserialize(Deserializer& s)
		{
			s >> name;
			s >> type;
			s >> filepath;
			s >> metadata;
		}
	};

	class IAssetCollector
	{
	public:
		virtual ~IAssetCollector() {}
		virtual void output(const String& name, AssetType type, const Bytes& data, Maybe<Metadata> metadata = {}) = 0;
		virtual void output(const String& name, AssetType type, gsl::span<const gsl::byte> data, Maybe<Metadata> metadata = {}) = 0;
		virtual void addAdditionalAsset(ImportingAsset&& asset) = 0;
		virtual bool reportProgress(float progress, const String& label = "") = 0;
		virtual Bytes readAdditionalFile(const Path& filePath) = 0;
		virtual const Path& getDestinationDirectory() = 0;
	};

	class IAssetImporter
	{
	public:
		virtual ~IAssetImporter() {}

		virtual ImportAssetType getType() const { return ImportAssetType::Skip; }
		virtual void import(const ImportingAsset&, IAssetCollector&) {}
		virtual int dropFrontCount() const { return 1; }

		virtual String getAssetId(const Path& file, const Maybe<Metadata>& metadata) const
		{
			String name = file.dropFront(dropFrontCount()).string();
			if (name.endsWith(".meta")) {
				name = name.substr(0, name.length() - 5);
			}
			return name;
		}
	};
}
