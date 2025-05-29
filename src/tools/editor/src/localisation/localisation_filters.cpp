#include "localisation_filters.h"

using namespace Halley;

void LocalisationFilters::initialise(bool translating)
{
	clearFilters();
	readyEnabled = translating;
}

bool LocalisationFilters::shouldShow(const LocalisationDataEntry& entry, const LocTranslationEntry* translation, const LocalisationFilterRules& rules) const
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

	if (readyEnabled) {
		if (entry.getReadyState(rules) != ready) {
			return false;
		}
	}

	return true;
}

bool LocalisationFilters::hasFiltersActive() const
{
	return outdatedEnabled || priorityEnabled || translatedEnabled || readyEnabled;
}

void LocalisationFilters::clearFilters()
{
	outdatedEnabled = false;
	priorityEnabled = false;
	translatedEnabled = false;
	readyEnabled = false;
}

String LocalisationFilters::toString() const
{
	Vector<String> filterStrings;
	if (hasFiltersActive()) {
		if (priorityEnabled) {
			filterStrings += "Priority between " + Halley::toString(minPriority) + " and " + Halley::toString(maxPriority);
		}
		if (readyEnabled) {
			if (ready == LocReadyStatus::Ready) {
				filterStrings += "Ready to translate";
			} else {
				filterStrings += "Not ready";
			}
		}
		if (translatedEnabled) {
			if (translated == LocTranslatedStatus::Translated) {
				filterStrings += "Translated";
			} else {
				filterStrings += "Untranslated";
			}
		}
		if (outdatedEnabled) {
			if (outdated == LocOutdatedStatus::OutOfDate) {
				filterStrings += "Out of Date";
			} else {
				filterStrings += "Up to Date";
			}
		}
	}

	if (filterStrings.empty()) {
		return "[No filters enabled]";
	} else {
		return String::concatList(filterStrings, ", ");
	}
}
