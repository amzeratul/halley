#pragma once
#include <memory>
#include "halley/data_structures/vector.h"
#include "halley/text/halleystring.h"
#include "halley/file/path.h"
#include "halley/utils/utils.h"
#include "halley/resources/metadata.h"
#include "halley/resources/resource.h"
#include "halley/data_structures/maybe.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/file_formats/config_file.h"

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
		Vector<ImportingAssetFile> inputFiles;
		ImportAssetType assetType = ImportAssetType::Undefined;
		ConfigNode options;
	};

	class AssetResource
	{
	public:
		struct PlatformVersion
		{
			String filepath;
			Metadata metadata;

			void serialize(Serializer& s) const
			{
				s << filepath;
				s << metadata;
			}
			
			void deserialize(Deserializer& s)
			{
				s >> filepath;
				s >> metadata;
			}
		};

		String name;
		AssetType type;
		std::map<String, PlatformVersion> platformVersions;
		Path primaryInputFile;

		void serialize(Serializer& s) const
		{
			s << name;
			s << type;
			s << platformVersions;
			s << primaryInputFile;
		}
		
		void deserialize(Deserializer& s)
		{
			s >> name;
			s >> type;
			s >> platformVersions;
			s >> primaryInputFile;
		}
	};

	class IAssetCollector
	{
	public:
		virtual ~IAssetCollector() {}
		virtual void output(const String& name, AssetType type, const Bytes& data, std::optional<Metadata> metadata = {}, const String& platform = "pc", const Path& primaryInputFile = {}) = 0;
		virtual void addAdditionalAsset(ImportingAsset&& asset) = 0;
		virtual bool reportProgress(float progress, const String& label = "") = 0;
		virtual Bytes readAdditionalFile(const Path& filePath) = 0;
		virtual const Path& getDestinationDirectory() = 0;
	};

	class IAssetImporter
	{
	public:
		virtual ~IAssetImporter() {}

		virtual ImportAssetType getType() const = 0;
		virtual void import(const ImportingAsset&, IAssetCollector&) {}
		virtual int dropFrontCount() const { return importByExtension ? 0 : 1; }

		virtual String getAssetId(const Path& file, const std::optional<Metadata>& metadata) const
		{
			return file.dropFront(dropFrontCount()).string();
		}

		void setImportByExtension(bool enabled)
		{
			importByExtension = enabled;
		}

	private:
		bool importByExtension = false;
	};

	class SkipAssetImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Skip; }
	};
}
