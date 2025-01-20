#pragma once
#include "localisation_grid.h"

namespace Halley {
	class LocalisationEditorRoot;
	class LocalisationData;

	class LocalisationLanguageEditor : public UIWidget {
    public:
        LocalisationLanguageEditor(LocalisationEditorRoot& root, Project& project, UIFactory& factory, LocalisationData& srcLanguage, LocalisationData* dstLanguage, bool canEdit);

		void onMakeUI() override;

	private:
		LocalisationEditorRoot& root;
		Project& project;
		UIFactory& factory;
		LocalisationData& srcLanguage;
		LocalisationData* dstLanguage;
		bool canEdit = false;

		LocalisationDataChunk* srcData = nullptr;
		LocalisationDataChunk* dstData = nullptr;

		std::shared_ptr<LocalisationGrid> grid;

		void setChunk(const String& chunkId);
		void setSelectedLine(int idx, const String& key);
    };
}
