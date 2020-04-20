#include "resource_filesystem.h"
#include "resources/resource_locator.h"
#include <halley/support/exception.h>
#include "resource_pack.h"
#include "halley/support/logger.h"
#include "api/system_api.h"

using namespace Halley;

ResourceLocator::ResourceLocator(SystemAPI& system)
	: system(system)
{
}

void ResourceLocator::add(std::unique_ptr<IResourceLocatorProvider> locator, const Path& path)
{
	loadLocatorData(*locator);
	locatorPaths[path.getString()] = locator.get();
	locators.emplace_back(std::move(locator));
}

void ResourceLocator::loadLocatorData(IResourceLocatorProvider& locator)
{
	auto& db = locator.getAssetDatabase();
	for (auto& asset: db.getAssets()) {
		auto result = assetToLocator.find(asset);
		if (result == assetToLocator.end() || result->second->getPriority() < locator.getPriority()) {
			assetToLocator[asset] = &locator;
		}
	}
}

void ResourceLocator::purge(const String& asset, AssetType type)
{
	auto result = assetToLocator.find(asset);
	if (result != assetToLocator.end()) {
		// Found the locator for this file, purge it
		result->second->purge(system);
	} else {
		// Couldn't find a locator (new file?), purge everything
		purgeAll();
	}
}

void ResourceLocator::purgeAll()
{
	assetToLocator.clear();
	for (auto& locator: locators) {
		locator->purge(system);
		loadLocatorData(*locator);
	}
}

std::unique_ptr<ResourceData> ResourceLocator::getResource(const String& asset, AssetType type, bool stream)
{
	auto result = assetToLocator.find(asset);
	if (result != assetToLocator.end()) {
		auto data = result->second->getData(asset, type, stream);
		if (data) {
			return data;
		} else {
			throw Exception("Unable to load resource: " + asset, HalleyExceptions::Resources);
		}
	} else {
		throw Exception("Unable to locate resource: " + asset, HalleyExceptions::Resources);
	}
}

std::unique_ptr<ResourceDataStatic> ResourceLocator::getStatic(const String& asset, AssetType type)
{
	auto rawPtr = getResource(asset, type, false).release();
	auto ptr = dynamic_cast<ResourceDataStatic*>(rawPtr);
	if (!ptr) {
		delete rawPtr;
		throw Exception("Resource " + asset + " obtained, but is not static data.", HalleyExceptions::Resources);
	}
	return std::unique_ptr<ResourceDataStatic>(ptr);
}

std::unique_ptr<ResourceDataStream> ResourceLocator::getStream(const String& asset, AssetType type)
{
	auto rawPtr = getResource(asset, type, true).release();
	auto ptr = dynamic_cast<ResourceDataStream*>(rawPtr);
	if (!ptr) {
		delete rawPtr;
		throw Exception("Resource " + asset + " obtained, but is not stream data.", HalleyExceptions::Resources);
	}
	return std::unique_ptr<ResourceDataStream>(ptr);
}

std::vector<String> ResourceLocator::enumerate(const AssetType type)
{
	std::vector<String> result;
	for (auto& l: locators) {
		for (auto& r: l->getAssetDatabase().enumerate(type)) {
			result.push_back(std::move(r));
		}
	}
	return result;
}

void ResourceLocator::addFileSystem(const Path& path)
{
	add(std::make_unique<FileSystemResourceLocator>(system, path), path);
}

void ResourceLocator::addPack(const Path& path, const String& encryptionKey, bool preLoad, bool allowFailure, std::optional<int> priority)
{
	auto dataReader = system.getDataReader(path.string());
	if (dataReader) {
		auto resourceLocator = std::make_unique<PackResourceLocator>(std::move(dataReader), path, encryptionKey, preLoad, priority);
		add(std::move(resourceLocator), path);

	} else {
		if (allowFailure) {
			Logger::logWarning("Resource pack not found: \"" + path.string() + "\"");
		} else {
			throw Exception("Unable to load resource pack \"" + path.string() + "\"", HalleyExceptions::Resources);
		}
	}
}

void ResourceLocator::removePack(const Path& path)
{
	auto* locatorToRemove = locatorPaths.find(path.getString())->second;
	auto& dbToRemove = locatorToRemove->getAssetDatabase();
	for (auto& asset : dbToRemove.getAssets()) {
		auto result = assetToLocator.find(asset);
		if (result != assetToLocator.end()) {
			assetToLocator.erase(asset);
		}
	}
	auto locaterIter = std::find_if(locators.begin(), locators.end(), [&](std::unique_ptr<IResourceLocatorProvider>& locator) { return locator.get() == locatorToRemove; });
	locators.erase(locaterIter);
	
	for (const auto& locator : locators)
	{
		auto& db = locator->getAssetDatabase();
		for (auto& asset : db.getAssets()) {
			auto result = assetToLocator.find(asset);
			if (result == assetToLocator.end() || result->second->getPriority() < locator->getPriority()) {
				assetToLocator[asset] = locator.get();
			}
		}
	}
}

std::vector<String> ResourceLocator::getAssetsFromPack(const Path& path, const String& encryptionKey) const
{
	auto dataReader = system.getDataReader(path.string());
	if (dataReader) {
		std::unique_ptr<IResourceLocatorProvider> resourceLocator = std::make_unique<PackResourceLocator>(std::move(dataReader), path, "", true);
		auto& db = resourceLocator->getAssetDatabase();
		return db.getAssets();
	}
	else {
		throw Exception("Unable to load resource pack \"" + path.string() + "\"", HalleyExceptions::Resources);
	}
}

const Metadata& ResourceLocator::getMetaData(const String& asset, AssetType type) const
{
	auto result = assetToLocator.find(asset);
	if (result != assetToLocator.end()) {
		return result->second->getAssetDatabase().getDatabase(type).get(asset).meta;
	} else {
		throw Exception("Unable to locate resource: " + asset, HalleyExceptions::Resources);
	}
}

bool ResourceLocator::exists(const String& asset)
{
	return assetToLocator.find(asset) != assetToLocator.end();
}

size_t ResourceLocator::getLocatorCount() const
{
	return assetToLocator.size();
}
