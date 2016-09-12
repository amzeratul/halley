#include "halley/resources/resource.h"

using namespace Halley;

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

void AsyncResource::waitForLoad()
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
