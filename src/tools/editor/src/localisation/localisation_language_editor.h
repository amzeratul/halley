#pragma once
#include "localisation_grid.h"

namespace Halley {
	class LocalisationEditorRoot;
	class LocOriginalData;

	class LocalisationLanguageEditor : public UIWidget {
    public:
        LocalisationLanguageEditor(LocalisationEditorRoot& root, Project& project, UIFactory& factory, LocOriginalData& srcLanguage, LocTranslationData* dstLanguage, LocOriginalData* srcRemote, LocTranslationData* locRemote, bool canEdit);

		void onMakeUI() override;
		void update(Time t, bool moved) override;

	private:
		LocalisationEditorRoot& root;
		Project& project;
		UIFactory& factory;
		LocOriginalData& srcLanguage;
		LocTranslationData* dstLanguage;
		LocOriginalData* srcRemote;
		LocTranslationData* locRemote;
		bool canEdit = false;
		bool acceptingTextInput = false;

		const ILocOriginalData* srcData = nullptr;
		HashMap<String, size_t> srcRemoteDataIndex;

		std::shared_ptr<LocalisationGrid> grid;

		String curEditingKey = "";

		void setChunk(const String& chunkId);
		void setSelectedLine(int idx, const String& key);

		void setSrcValue(const String& value);
		void setDstValue(const String& value);
		void setComment(const String& comment);
		void setContext(const String& context);
		void setPriority(LocPriority priority);
    };
}
