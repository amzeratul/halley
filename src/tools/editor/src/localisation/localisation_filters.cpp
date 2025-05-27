#include "localisation_filters.h"

using namespace Halley;

bool LocalisationFilters::shouldShow(const LocalisationDataEntry& entry, const LocTranslationEntry* translation) const
{
	if (!searchString.isEmpty()) {
		if (!entry.value.contains(searchString) && !entry.key.contains(searchString)) {
			return false;
		}
	}

	if (priorityEnabled) {
		if (entry.priority < minPriority || entry.priority > maxPriority) {
			return false;
		}
	}

	const bool isTranslated = translation && !translation->value.isEmpty();

	if (outdatedEnabled) {
		const bool isOutOfDate = isTranslated && translation->origVersion < entry.version;
		const bool wantsOutOfDate = outdated == LocOutdatedStatus::OutOfDate;
		if (isOutOfDate != wantsOutOfDate) {
			return false;
		}
	}

	if (translatedEnabled) {
		const bool wantsTranslated = translated == LocTranslatedStatus::Translated;
		if (isTranslated != wantsTranslated) {
			return false;
		}
	}

	return true;
}

bool LocalisationFilters::hasFiltersActive() const
{
	return outdatedEnabled || priorityEnabled || translatedEnabled;
}

void LocalisationFilters::clearFilters()
{
	outdatedEnabled = false;
	priorityEnabled = false;
	translatedEnabled = false;
}
