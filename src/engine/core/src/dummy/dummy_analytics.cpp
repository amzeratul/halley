#include "dummy_analytics.h"
using namespace Halley;

void DummyAnalyticsAPI::init()
{
}

void DummyAnalyticsAPI::deInit()
{
}

void DummyAnalyticsAPI::setUserOptIn(bool userHasOptIn)
{
}

void DummyAnalyticsAPI::reportResourceEvent(
		ResourceType type,
		const String& itemType,
		const String& itemId,
		const String& currency,
		int amount) const
{
}

void DummyAnalyticsAPI::reportProgressionEvent(
		ProgressionType type,
		const String& id1,
		std::optional<String> id2,
		std::optional<String> id3) const
{
}

void DummyAnalyticsAPI::reportDesignEvent(String eventId) const
{
}
