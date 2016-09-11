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

ResourceDataStatic::ResourceDataStatic(void* _data, size_t _size, String path)
	: ResourceData(path)
	, loaded(false)
{
	set(_data, _size);
}

static void deleter(char* data)
{
	delete[] data;
}

void ResourceDataStatic::set(void* _data, size_t _size)
{
	data = std::shared_ptr<char>(static_cast<char*>(_data), deleter);
	size = _size;
	loaded = true;
}

void* ResourceDataStatic::getData()
{
	if (!loaded) throw Exception("Resource data not yet loaded");
	return data.get();
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

ResourceLoader::ResourceLoader(IResourceLocator& locator, String name, String resolvedName, ResourceLoadPriority priority, HalleyAPI* api, std::unique_ptr<Metadata> metadata)
	: locator(locator)
	, name(name)
	, resolvedName(resolvedName)
	, priority(priority)
	, api(api)
	, metadata(move(metadata))
{}

String ResourceLoader::getBasePath() const
{
	auto basePath = getName();
	size_t lastSlash = basePath.find_last_of('/');
	if (lastSlash != std::string::npos) {
		basePath = basePath.left(lastSlash + 1);
	};
	return basePath;
}

std::unique_ptr<ResourceDataStatic> ResourceLoader::getStatic()
{
	auto result = locator.getStatic(resolvedName);
	if (result) {
		loaded = true;
	}
	return result;
}

std::unique_ptr<ResourceDataStream> ResourceLoader::getStream()
{
	auto result = locator.getStream(resolvedName);
	if (result) {
		loaded = true;
	}
	return result;
}

Future<std::unique_ptr<ResourceDataStatic>> ResourceLoader::getAsync() const
{
	std::reference_wrapper<IResourceLocator> loc = locator;
	String resName = resolvedName;
	return Concurrent::execute(Executors::getDiskIO(), [loc, resName] () -> std::unique_ptr<ResourceDataStatic>
	{
		return loc.get().getStatic(resName);
	});
}
