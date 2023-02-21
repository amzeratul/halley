#pragma once
#include "api/halley_api_internal.h"

namespace Halley
{
	class DummyAnalyticsAPI : public AnalyticsAPIInternal
	{
	public:
		void init() override;
		void deInit() override;

		void setUserOptIn(bool userHasOptIn) override;

		virtual void reportResourceEvent(
				ResourceType type,
				const String& itemType,
				const String& itemId,
				const String& currency,
				int amount) const override;

		void reportProgressionEvent(
				ProgressionType type,
				const String& id1,
				std::optional<String> id2,
				std::optional<String> id3) const override;

		void reportDesignEvent(String eventId) const override;
	};
}
