/*****************************************************************\
__
/ /
/ /                     __  __
/ /______    _______    / / / / ________   __       __
/ ______  \  /_____  \  / / / / / _____  | / /      / /
/ /      | / _______| / / / / / / /____/ / / /      / /
/ /      / / / _____  / / / / / / _______/ / /      / /
/ /      / / / /____/ / / / / / / |______  / |______/ /
/_/      /_/ |________/ / / / /  \_______/  \_______  /
/_/ /_/                     / /
/ /
High Level Game Framework            /_/

---------------------------------------------------------------

Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include "halley/text/halleystring.h"
#include "halley/file/path.h"
#include <memory>
#include <functional>
#include <halley/concurrency/future.h>
#include <gsl/gsl>
#include "metadata.h"

namespace Halley {
	enum class AssetType;

	class ResourceDataReader {
	public:
		virtual ~ResourceDataReader() {}
		virtual size_t size() const = 0;
		virtual int read(gsl::span<gsl::byte> dst) = 0;
		virtual void seek(int64_t pos, int whence) = 0;
		virtual size_t tell() const = 0;
		virtual void close() = 0;
	};

	class ResourceData {
	public:
		ResourceData(String path);
		virtual ~ResourceData() {}

		String getPath() const { return path; }

	private:
		String path;
	};

	class ResourceDataStatic : public ResourceData {
	public:
		ResourceDataStatic(String path);
		ResourceDataStatic(const void* data, size_t size, String path, bool owning = true);

		void set(const void* data, size_t size, bool owning = true);
		bool isLoaded() const;

		const void* getData() const;
		gsl::span<const gsl::byte> getSpan() const;
		size_t getSize() const;
		String getString() const;
		void inflate();

		static std::unique_ptr<ResourceDataStatic> loadFromFileSystem(Path path);
		void writeToFileSystem(String path) const;

	private:
		std::shared_ptr<const char> data;
		size_t size = 0;
		bool loaded;
	};

	typedef std::function<std::unique_ptr<ResourceDataReader>()> ResourceDataMakeReader;
	class ResourceDataStream : public ResourceData {
	public:
		ResourceDataStream(String path, ResourceDataMakeReader makeReader);
		std::unique_ptr<ResourceDataReader> getReader() const { return make(); }

	private:
		ResourceDataMakeReader make;
	};

	class IResourceLocator
	{
	public:
		virtual ~IResourceLocator() {}
		virtual const Metadata& getMetaData(const String& resource, AssetType type) const = 0;
		virtual std::unique_ptr<ResourceDataStatic> getStatic(const String& asset, AssetType type) = 0;
		virtual std::unique_ptr<ResourceDataStream> getStream(const String& asset, AssetType type) = 0;
	};


	enum class ResourceLoadPriority {
		Low = 0,
		Normal = 1,
		High = 2
	};

	class HalleyAPI;
	class Metadata;

	class ResourceLoader
	{
		friend class ResourceCollectionBase;

	public:
		const String& getName() const { return name; }
		ResourceLoadPriority getPriority() const { return priority; }
		HalleyAPI& getAPI() const { return *api; }
		const Metadata& getMeta() const { return *metadata; }

		std::unique_ptr<ResourceDataStatic> getStatic();
		std::unique_ptr<ResourceDataStream> getStream();
		Future<std::unique_ptr<ResourceDataStatic>> getAsync() const;

	private:
		ResourceLoader(ResourceLoader&& loader) noexcept;
		ResourceLoader(IResourceLocator& locator, const String& name, AssetType type, ResourceLoadPriority priority, HalleyAPI* api);
		~ResourceLoader();

		IResourceLocator& locator;
		String name;
		AssetType type;
		ResourceLoadPriority priority;
		HalleyAPI* api;
		const Metadata* metadata;
		bool loaded = false;
	};

}
