#include "localisation_filters.h"

using namespace Halley;

bool LocalisationFilters::shouldShow(const LocalisationDataEntry& entry, const LocTranslationEntry* translation) const
{
	if (!searchString.isEmpty()) {
		if (entry.value.contains(searchString) && entry.key.contains(searchString)) {
			return false;
		}
	}

	if (priorityEnabled) {
		if (entry.priority < minPriority || entry.priority > maxPriority) {
			return false;
		}
	}

	if (translation) {
		const bool isTranslated = !translation->value.isEmpty();

		if (outdatedEnabled) {
			if (!isTranslated) { // If it's empty, ignore this check - we want to match it either way
				const bool isOutOfDate = translation->origVersion < entry.version;
				const bool wantsOutOfDate = outdated == LocOutdatedStatus::OutOfDate;
				if (isOutOfDate != wantsOutOfDate) {
					return false;
				}
			}
		}

		if (translatedEnabled) {
			const bool wantsTranslated = translated == LocTranslatedStatus::Translated;
			if (isTranslated != wantsTranslated) {
				return false;
			}
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
