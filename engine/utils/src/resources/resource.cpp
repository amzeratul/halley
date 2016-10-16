#include "halley/resources/resource.h"

using namespace Halley;

void Resource::setMeta(const Metadata& m)
{
	meta = m;
}

const Metadata& Resource::getMeta() const
{
	return meta;
}

AsyncResource::AsyncResource() 
	: loading(false)
{}

AsyncResource::~AsyncResource()
{
	waitForLoad();
}

void AsyncResource::startLoading()
{
	loading = true;
}

void AsyncResource::doneLoading()
{
	loading = false;
	loadWait.notify_all();
}

void AsyncResource::waitForLoad() const
{
	if (loading) {
		std::unique_lock<std::mutex> lock(loadMutex);
		while (loading) {
			loadWait.wait(lock);
		}
	}
}

bool AsyncResource::isLoaded() const
{
	return !loading;
}
