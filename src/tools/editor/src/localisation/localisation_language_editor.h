#pragma once

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
    };
}
