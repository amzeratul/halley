#include "dummy_analytics.h"
using namespace Halley;

void DummyAnalyticsAPI::init()
{
}

void DummyAnalyticsAPI::deInit()
{
}

void DummyAnalyticsAPI::setLocalLogPath(const Halley::String& path)
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
		std::optional<String> id3,
		int score) const
{
}

void DummyAnalyticsAPI::reportDesignEvent(
		const String& eventId,
		double value) const
{
}

void DummyAnalyticsAPI::reportErrorEvent(
		ErrorType severity,
		const String& message) const
{
}