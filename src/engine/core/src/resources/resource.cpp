#include "halley/resources/resource.h"
#include "halley/support/exception.h"

using namespace Halley;

Resource::~Resource() = default;

void Resource::setMeta(Metadata m)
{
	// Only allow setting meta once to avoid a race condition
	// For example, resource_collection.cpp would try to set a meta on a Texture while texture.cpp is trying to load it and referencing its data
	if (!metaSet) {
		meta = std::move(m);
		metaSet = true;
	}
}

void Resource::setAssetId(String id)
{
	assetId = std::move(id);
}

void Resource::onLoaded(Resources& resources)
{
}

void Resource::increaseAssetVersion()
{
	++assetVersion;
}

void Resource::reloadResource(Resource&& resource)
{
	++assetVersion;
	resource.assetVersion = assetVersion;
	reload(std::move(resource));
}

ResourceMemoryUsage Resource::getMemoryUsage() const
{
	return ResourceMemoryUsage{};
}

void Resource::increaseAge(float time)
{
	age += time;
}

void Resource::resetAge()
{
	age = 0;
}

float Resource::getAge() const
{
	return age;
}

void Resource::startFrame(Time dt) const
{
}

void Resource::setUnloaded()
{
	unloaded = true;
}

bool Resource::isUnloaded() const
{
	return unloaded;
}

void Resource::onOtherResourcesUnloaded()
{
}

void Resource::reload(Resource&& resource)
{
}

ResourceObserver::ResourceObserver()
{
}

ResourceObserver::ResourceObserver(const Resource& res)
{
	startObserving(res);
}

ResourceObserver::~ResourceObserver()
{
	stopObserving();
}

void ResourceObserver::startObserving(const Resource& r)
{
	res = &r;
	assetVersion = res->getAssetVersion();
}

void ResourceObserver::stopObserving()
{
	res = nullptr;
	assetVersion = 0;
}

bool ResourceObserver::needsUpdate() const
{
	return res && res->getAssetVersion() != assetVersion;
}

void ResourceObserver::update()
{
	assetVersion = res->getAssetVersion();
}

const Resource* ResourceObserver::getResourceBeingObserved() const
{
	return res;
}

AsyncResource::AsyncResource() 
	: failed(false)
	, loadState(State::Unloaded)
	, inUseThisFrame(false)
	, inBackgroundThisFrame(false)
{}

AsyncResource::~AsyncResource()
{
	waitForLoad(true);
}

AsyncResource::AsyncResource(const AsyncResource& other)
	: Resource(other)
{
	other.waitForLoad(true);
	*this = other;
}

AsyncResource::AsyncResource(AsyncResource&& other) noexcept
{
	other.waitForLoad(true);
	*this = std::move(other);
}

AsyncResource& AsyncResource::operator=(const AsyncResource& other)
{
	other.waitForLoad(true);
	failed.store(other.failed);
	Resource::operator=(static_cast<const Resource&>(other));
	return *this;
}

AsyncResource& AsyncResource::operator=(AsyncResource&& other) noexcept
{
	other.waitForLoad(true);
	failed.store(other.failed);
	Resource::operator=(static_cast<Resource&&>(other));
	return *this;
}

void AsyncResource::startLoading()
{
	if (loadState == State::Unloaded) {
		loadState = State::Loading;
		failed = false;
	}
}

void AsyncResource::doneLoading()
{
	if (loadState != State::Loaded) {
		Vector<Promise<void>> promises;
		{
			UniqueLock lock(loadMutex);
			loadState = State::Loaded;
			promises = std::move(pendingPromises);
		}
		loadWait.notifyAll();
		for (auto& p: promises) {
			p.set();
		}
	}
}

void AsyncResource::loadingFailed()
{
	failed = true;
	doneLoading();
}

void AsyncResource::waitForLoad(bool acceptFailed) const
{
	requestLoading();
	if (loadState == State::Loading) {
		UniqueLock lock(loadMutex);
		while (loadState != State::Loaded) {
			loadWait.wait(lock);
		}
	}
	if (failed && !acceptFailed) {
		throw Exception("Resource failed to load.", HalleyExceptions::Resources);
	}
}

Future<void> AsyncResource::onLoad() const
{
	UniqueLock lock(loadMutex);
	if (loadState != State::Loaded) {
		pendingPromises.push_back({});
		return pendingPromises.back().getFuture();
	} else {
		return Future<void>::makeImmediate({});
	}
}

void AsyncResource::startFrame(Time dt) const
{
	usageData.timeSinceInUse += dt;
	usageData.timeSinceInBackground += dt;
	usageData.framesSinceInUse++;
	usageData.framesSinceInBackground++;
	usageData.loaded = loadState == State::Loaded;

	if (inBackgroundThisFrame.load(std::memory_order_relaxed)) {
		usageData.timeSinceInBackground = 0;
		usageData.framesSinceInBackground = 0;
	}
	if (inUseThisFrame.load(std::memory_order_relaxed)) {
		usageData.timeSinceInUse = 0;
		usageData.framesSinceInBackground = 0;
	}

	inUseThisFrame.store(false, std::memory_order_relaxed);
	inBackgroundThisFrame.store(false, std::memory_order_relaxed);
}

void AsyncResource::markActivelyInUse() const
{
	inUseThisFrame.store(true, std::memory_order_relaxed);
	requestLoading();
}

void AsyncResource::markBackgroundLoaded() const
{
	inBackgroundThisFrame.store(true, std::memory_order_relaxed);
}

const AsyncResource::UsageData& AsyncResource::getUsageData() const
{
	return usageData;
}

bool AsyncResource::isLoaded() const
{
	return loadState == State::Loaded;
}

bool AsyncResource::hasSucceeded() const
{
	return !failed;
}

bool AsyncResource::hasFailed() const
{
	return failed;
}

void AsyncResource::requestLoading() const
{
	const_cast<AsyncResource*>(this)->requestLoading();
}

void AsyncResource::requestLoading()
{
	if (loadState == State::Unloaded) {
		UniqueLock lock(loadMutex);
		doRequestLoading();
	}
}

void AsyncResource::doRequestLoading()
{
}
