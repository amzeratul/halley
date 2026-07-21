#pragma once

#include <halley.hpp>
#include "localisation_data.h"

namespace Halley {
	enum class LocTranslatedStatus {
		Untranslated,
		Translated
	};

	template <>
	struct EnumNames<LocTranslatedStatus> {
		constexpr auto operator()() const {
			return std::to_array({
				"untranslated",
				"translated"
			});
		}
	};

	enum class LocOutdatedStatus {
		UpToDate,
		OutOfDate
	};

	template <>
	struct EnumNames<LocOutdatedStatus> {
		constexpr auto operator()() const {
			return std::to_array({
				"upToDate",
				"outOfDate"
			});
		}
	};

	enum class LocCommentStatus {
		HasComment,
		NoComment
	};

	template <>
	struct EnumNames<LocCommentStatus> {
		constexpr auto operator()() const {
			return std::to_array({
				"hasComment",
				"noComment"
			});
		}
	};

	enum class LocContextStatus {
		HasContext,
		NoContext
	};

	template <>
	struct EnumNames<LocContextStatus> {
		constexpr auto operator()() const {
			return std::to_array({
				"hasContext",
				"noContext"
			});
		}
	};

	class LocalisationFilters {
	public:
		String searchString;

		bool priorityEnabled = false;
		bool outdatedEnabled = false;
		bool translatedEnabled = false;
		bool readyEnabled = false;
		bool commentEnabled = false;
		bool contextEnabled = false;

		LocOutdatedStatus outdated = LocOutdatedStatus::OutOfDate;
		LocTranslatedStatus translated = LocTranslatedStatus::Untranslated;
		LocReadyStatus ready = LocReadyStatus::Ready;
		LocPriority minPriority = LocPriority::Lowest;
		LocPriority maxPriority = LocPriority::Highest;
		LocCommentStatus comment = LocCommentStatus::HasComment;
		LocContextStatus context = LocContextStatus::HasContext;

		void initialise(bool translating);

		bool shouldShow(const LocalisationDataEntry& entry, const LocTranslationEntry* translation, const LocalisationFilterRules& rules, const I18NLanguage& language) const;
		bool hasFiltersActive() const;
		void clearFilters();

		String toString() const;
	};

    struct LocalisationExportOptions {
		LocalisationFilters filters;
        bool allChunks = true;
		bool includeBOM = false;
        HashSet<String> chunksToInclude;
    };
}
