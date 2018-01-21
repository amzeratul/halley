#include "halley/resources/resource.h"
#include "halley/support/exception.h"

using namespace Halley;

void Resource::setMeta(const Metadata& m)
{
	meta = m;
}

const Metadata& Resource::getMeta() const
{
	return meta;
}

void Resource::setAssetId(const String& n)
{
	assetId = n;
}

const String& Resource::getAssetId() const
{
	return assetId;
}

AsyncResource::AsyncResource() 
	: failed(false)
	, loading(false)
{}

AsyncResource::~AsyncResource()
{
	waitForLoad();
}

void AsyncResource::startLoading()
{
	loading = true;
	failed = false;
}

void AsyncResource::doneLoading()
{
	{
		std::unique_lock<std::mutex> lock(loadMutex);
		loading = false;
	}
	loadWait.notify_all();
}

void AsyncResource::loadingFailed()
{
	failed = true;
	doneLoading();
}

void AsyncResource::waitForLoad() const
{
	if (loading) {
		std::unique_lock<std::mutex> lock(loadMutex);
		while (loading) {
			loadWait.wait(lock);
		}
	}
	if (failed) {
		throw Exception("Resource failed to load.");
	}
}

bool AsyncResource::isLoaded() const
{
	return !loading;
}
