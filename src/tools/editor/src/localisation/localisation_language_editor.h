#pragma once
#include "localisation_filters.h"
#include "localisation_grid.h"

namespace Halley {
	class LocalisationEditorRoot;
	class LocOriginalData;
	class LocalisationClient;

	class LocalisationLanguageEditor : public UIWidget {
    public:
        LocalisationLanguageEditor(LocalisationEditorRoot& root, LocalisationClient& client, Project& project, UIFactory& factory, const HalleyAPI& api, LocOriginalData& srcLanguage, LocTranslationData* dstLanguage, LocOriginalData* srcRemote, LocTranslationData* locRemote, bool canEdit);

		void onMakeUI() override;
		void update(Time t, bool moved) override;

	private:
		LocalisationEditorRoot& root;
		LocalisationClient& client;
		Project& project;
		UIFactory& factory;
		const HalleyAPI& api;

		LocOriginalData& srcLanguage;
		LocTranslationData* dstLanguage;
		LocOriginalData* srcRemote;
		LocTranslationData* locRemote;
		bool canEdit = false;
		bool acceptingTextInput = false;

		AliveFlag aliveFlag;

		const ILocOriginalData* srcData = nullptr;
		HashMap<String, size_t> srcRemoteDataIndex;

		std::shared_ptr<LocalisationGrid> grid;
		LocalisationFilters filters;
		LocalisationFilterRules filterRules;

		String curEditingKey = "";
		Vector<String> pendingTranslationModifiedKeys;
		Vector<String> pendingTranslationTextEditedKeys;
		bool uploadingKeys = false;

		void setChunk(const String& chunkId);
		void setSelectedLine(int idx, const String& key);

		void setSrcValue(const String& value);
		void setDstValue(const String& value);
		void setComment(const String& comment);
		void setContext(const String& context);
		void setPriority(LocPriority priority);

		void markUpToDate();
		void markOutOfDate();
		std::pair<int, int> getOutOfDateAndUpToDateCountInSelection() const;

		void onStringPropertiesModified(const Vector<String>& keys);

		void uploadPendingTranslations(bool force);
		void close();

		void setFilters();
		void onFiltersUpdated();
		void updateFilterDisplay();
		void applyFilters();

		std::optional<Colour4f> getRowColour(int idx) const;
		bool isRowVisible(int idx) const;
    };
}
