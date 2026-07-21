#include "localisation_filters.h"

using namespace Halley;

void LocalisationFilters::initialise(bool translating)
{
	clearFilters();
	readyEnabled = translating;
}

bool LocalisationFilters::shouldShow(const LocalisationDataEntry& entry, const LocTranslationEntry* translation, const LocalisationFilterRules& rules, const I18NLanguage& language) const
{
	if (!searchString.isEmpty()) {
		if (!entry.matchesSearchString(searchString, translation)) {
			return false;
		}
	}

	if (priorityEnabled) {
		if (entry.getPriority() < minPriority || entry.getPriority() > maxPriority) {
			return false;
		}
	}

	const bool isTranslated = translation && !translation->getValue().isEmpty();

	if (outdatedEnabled) {
		const bool isOutOfDate = isTranslated && translation->origVersion < entry.getVersion();
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
		if (entry.getReadyState(rules, language) != ready) {
			return false;
		}
	}

	if (commentEnabled) {
		const bool wantsComment = comment == LocCommentStatus::HasComment;
		const bool hasComment = !entry.getComment().isEmpty();
		if (wantsComment != hasComment) {
			return false;
		}
	}

	if (contextEnabled) {
		const bool wantsContext = context == LocContextStatus::HasContext;
		const bool hasContext = !entry.getContext().isEmpty();
		if (wantsContext != hasContext) {
			return false;
		}
	}

	return true;
}

bool LocalisationFilters::hasFiltersActive() const
{
	return outdatedEnabled || priorityEnabled || translatedEnabled || readyEnabled || commentEnabled || contextEnabled;
}

void LocalisationFilters::clearFilters()
{
	outdatedEnabled = false;
	priorityEnabled = false;
	translatedEnabled = false;
	readyEnabled = false;
	commentEnabled = false;
	contextEnabled = false;
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
		if (commentEnabled) {
			if (comment == LocCommentStatus::HasComment) {
				filterStrings += "Has Comment";
			} else {
				filterStrings += "Doesn't Have Comment";
			}
		}
		if (contextEnabled) {
			if (context == LocContextStatus::HasContext) {
				filterStrings += "Has Context";
			} else {
				filterStrings += "Doesn't Have Context";
			}
		}
	}

	if (filterStrings.empty()) {
		return "[No filters enabled]";
	} else {
		return String::concatList(filterStrings, ", ");
	}
}
