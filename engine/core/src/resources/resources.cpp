#include "resources/resources.h"
#include "resources/resource_locator.h"
#include "api/halley_api.h"

using namespace Halley;

Resources::Resources(std::unique_ptr<ResourceLocator> locator, HalleyAPI* api)
	: locator(std::move(locator))
	, api(api)
	, basePath(api->system->getResourcesBasePath())
{}

Resources::~Resources() = default;

void Resources::setBasePath(const String& path)
{
	basePath = path;
}

String Resources::getBasePath() const 
{
	return basePath;
}

void Resources::setDepth(int depth) 
{
	curDepth = depth;
}
