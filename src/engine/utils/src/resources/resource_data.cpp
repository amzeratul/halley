#include <fstream>
#include "halley/resources/resource_data.h"
#include "halley/resources/metadata.h"
#include "halley/support/exception.h"
#include <halley/concurrency/concurrent.h>
#include "halley/file/compression.h"

using namespace Halley;

Bytes ResourceDataReader::readAll()
{
	Bytes result(size() - tell());
	read(gsl::as_writeable_bytes(gsl::span<Byte>(result)));
	return result;
}

ResourceData::ResourceData(String p)
	: path(p)
{
}

ResourceDataStatic::ResourceDataStatic(String path)
	: ResourceData(path)
	, loaded(false)
{
}

ResourceDataStatic::ResourceDataStatic(const void* _data, size_t _size, String path, bool owning)
	: ResourceData(path)
	, loaded(false)
{
	set(_data, _size, owning);
}

static void deleter(const char* data)
{
	delete[] data;
}

static void noOpDeleter(const char*)
{
}

void ResourceDataStatic::set(const void* _data, size_t _size, bool owning)
{
	data = std::shared_ptr<const char>(static_cast<const char*>(_data), owning ? deleter : noOpDeleter);
	size = _size;
	loaded = true;
}

const void* ResourceDataStatic::getData() const
{
	if (!loaded) throw Exception("Resource data not yet loaded", HalleyExceptions::Resources);
	return data.get();
}

gsl::span<const gsl::byte> ResourceDataStatic::getSpan() const
{
	return gsl::span<const gsl::byte>(reinterpret_cast<const gsl::byte*>(getData()), getSize());
}

size_t ResourceDataStatic::getSize() const
{
	if (!loaded) throw Exception("Resource data not yet loaded", HalleyExceptions::Resources);
	return size;
}

bool ResourceDataStatic::isLoaded() const
{
	return loaded;
}

String ResourceDataStatic::getString() const
{
	return String(static_cast<const char*>(getData()), getSize());
}

void ResourceDataStatic::inflate()
{
	data = Compression::decompressToSharedPtr(getSpan(), size);
}

std::unique_ptr<ResourceDataStatic> ResourceDataStatic::loadFromFileSystem(Path path)
{
	std::ifstream fp(path.string(), std::ios::binary | std::ios::in);
	if (!fp) {
		return {};
	}

	fp.seekg(0, std::ios::end);
	const size_t size = size_t(fp.tellg());
	fp.seekg(0, std::ios::beg);
	char* buffer = new char[size]; // ResourceDataStatic will own this
	try {
		fp.read(buffer, size);
		fp.close();
		return std::make_unique<ResourceDataStatic>(buffer, size, path.string());
	} catch (...) {
		delete[] buffer; // ...unless it fails, so delete it before re-throwing error
		throw;
	}
}

void ResourceDataStatic::writeToFileSystem(String filePath) const
{
	std::ofstream fp(filePath, std::ios::binary | std::ios::out);
	fp.write(data.get(), size);
	fp.close();
}

ResourceDataStream::ResourceDataStream(String path, ResourceDataMakeReader makeReader)
	: ResourceData(path)
	, make(makeReader)
{
}

ResourceLoader::ResourceLoader(ResourceLoader&& loader) noexcept
	: locator(loader.locator)
	, name(std::move(loader.name))
	, priority(loader.priority)
	, api(loader.api)
{
}

ResourceLoader::~ResourceLoader()
{
}

ResourceLoader::ResourceLoader(IResourceLocator& locator, const String& name, AssetType type, ResourceLoadPriority priority, const HalleyAPI* api)
	: locator(locator)
	, name(name)
	, type(type)
	, priority(priority)
	, api(api)
{
	metadata = &locator.getMetaData(name, type);
}

std::unique_ptr<ResourceDataStatic> ResourceLoader::getStatic()
{
	auto result = locator.getStatic(name, type);
	if (result) {
		if (metadata->getString("asset_compression", "") == "deflate") {
			try {
				result->inflate();
			} catch (Exception &e) {
				throw Exception("Failed to load resource \"" + getName() + "\" due to inflate exception: " + e.what(), HalleyExceptions::Resources);
			}
		}
		loaded = true;
	}
	return result;
}

std::unique_ptr<ResourceDataStream> ResourceLoader::getStream()
{
	auto result = locator.getStream(name, type);
	if (result) {
		loaded = true;
	}
	return result;
}

Future<std::unique_ptr<ResourceDataStatic>> ResourceLoader::getAsync() const
{
	std::reference_wrapper<IResourceLocator> loc = locator;
	auto n = name;
	auto t = type;
	auto meta = getMeta();
	return Concurrent::execute(Executors::getDiskIO(), [meta, loc, n, t] () -> std::unique_ptr<ResourceDataStatic>
	{
		auto result = loc.get().getStatic(n, t);
		if (meta.getString("asset_compression", "") == "deflate") {
			result->inflate();
		}
		return result;
	});
}
