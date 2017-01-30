#include <fstream>
#include "halley/resources/resource_data.h"
#include "halley/resources/metadata.h"
#include "halley/support/exception.h"
#include <halley/concurrency/concurrent.h>

using namespace Halley;

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
	if (!loaded) throw Exception("Resource data not yet loaded");
	return data.get();
}

gsl::span<const gsl::byte> ResourceDataStatic::getSpan() const
{
	return gsl::span<const gsl::byte>(reinterpret_cast<const gsl::byte*>(getData()), getSize());
}

size_t ResourceDataStatic::getSize() const
{
	if (!loaded) throw Exception("Resource data not yet loaded");
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

std::unique_ptr<ResourceDataStatic> ResourceDataStatic::loadFromFileSystem(Path path)
{
	std::ifstream fp(path.string(), std::ios::binary | std::ios::in);
	fp.seekg(0, std::ios::end);
	size_t size = fp.tellg();
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

ResourceLoader::ResourceLoader(ResourceLoader&& loader)
	: locator(loader.locator)
	, name(std::move(loader.name))
	, priority(loader.priority)
	, api(loader.api)
{
}

ResourceLoader::~ResourceLoader()
{
}

ResourceLoader::ResourceLoader(IResourceLocator& locator, String name, Path path, ResourceLoadPriority priority, HalleyAPI* api, std::unique_ptr<Metadata> metadata)
	: locator(locator)
	, name(name)
	, path(path)
	, priority(priority)
	, api(api)
	, metadata(move(metadata))
{}

std::unique_ptr<ResourceDataStatic> ResourceLoader::getStatic()
{
	auto result = locator.getStatic(path.string());
	if (result) {
		loaded = true;
	}
	return result;
}

std::unique_ptr<ResourceDataStream> ResourceLoader::getStream()
{
	auto result = locator.getStream(path.string());
	if (result) {
		loaded = true;
	}
	return result;
}

Future<std::unique_ptr<ResourceDataStatic>> ResourceLoader::getAsync() const
{
	std::reference_wrapper<IResourceLocator> loc = locator;
	auto p = path;
	return Concurrent::execute(Executors::getDiskIO(), [loc, p] () -> std::unique_ptr<ResourceDataStatic>
	{
		return loc.get().getStatic(p.string());
	});
}
