#pragma once

namespace Halley {

	class AnalyticsAPI
	{
	public:
		virtual ~AnalyticsAPI() = default;

		enum class ResourceType
		{
			Gain,
			Sink
		};

		enum class ProgressionType
		{
			Start,
			Complete,
			Fail
		};

		enum class ErrorType
		{
			Debug,
			Info,
			Warning,
			Error,
			Critical
		};

		virtual void setLocalLogPath(const String& path) = 0;

		// Implementations should not collect or send any data before user consent
		// has been confirmed, and this was called with userHasOptIn == true.
		virtual void setUserOptIn(bool userHasOptIn) = 0;

		virtual void reportResourceEvent(
				ResourceType type,
				const String& itemType,
				const String& itemId,
				const String& currency,
				int amount) const = 0;

		virtual void reportProgressionEvent(
				ProgressionType type,
				const String& id1,
				std::optional<String> id2,
				std::optional<String> id3,
				int score) const = 0;

		virtual void reportDesignEvent(
				const String& eventId,
				double value) const = 0;

		virtual void reportErrorEvent(
				ErrorType severity,
				const String& message) const = 0;
	};

}
