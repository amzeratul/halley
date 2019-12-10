#include "resources/resources.h"
#include "resources/resource_locator.h"
#include "api/halley_api.h"

using namespace Halley;

Resources::Resources(std::unique_ptr<ResourceLocator> locator, const HalleyAPI& api)
	: locator(std::move(locator))
	, api(&api)
{
}

Resources::~Resources() = default;
