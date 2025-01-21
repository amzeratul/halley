#pragma once
#include "localisation_grid.h"

namespace Halley {
	class LocalisationEditorRoot;
	class LocOriginalData;

	class LocalisationLanguageEditor : public UIWidget {
    public:
        LocalisationLanguageEditor(LocalisationEditorRoot& root, Project& project, UIFactory& factory, LocOriginalData& srcLanguage, LocTranslationData* dstLanguage, bool canEdit);

		void onMakeUI() override;

	private:
		LocalisationEditorRoot& root;
		Project& project;
		UIFactory& factory;
		LocOriginalData& srcLanguage;
		LocTranslationData* dstLanguage;
		bool canEdit = false;
		bool acceptingTextInput = false;

		const ILocOriginalData* srcData = nullptr;

		std::shared_ptr<LocalisationGrid> grid;

		String curEditingKey = "";

		void setChunk(const String& chunkId);
		void setSelectedLine(int idx, const String& key);
    };
}
