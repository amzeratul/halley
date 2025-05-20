#pragma once
#include "localisation_grid.h"

namespace Halley {
	class LocalisationEditorRoot;
	class LocOriginalData;
	class LocalisationClient;

	class LocalisationLanguageEditor : public UIWidget {
    public:
        LocalisationLanguageEditor(LocalisationEditorRoot& root, LocalisationClient& client, Project& project, UIFactory& factory, LocOriginalData& srcLanguage, LocTranslationData* dstLanguage, LocOriginalData* srcRemote, LocTranslationData* locRemote, bool canEdit);

		void onMakeUI() override;
		void update(Time t, bool moved) override;

	private:
		LocalisationEditorRoot& root;
		LocalisationClient& client;
		Project& project;
		UIFactory& factory;
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

		String curEditingKey = "";
		Vector<String> pendingTranslationModifiedKeys;
		bool uploadingKeys = false;

		void setChunk(const String& chunkId);
		void setSelectedLine(int idx, const String& key);

		void setSrcValue(const String& value);
		void setDstValue(const String& value);
		void setComment(const String& comment);
		void setContext(const String& context);
		void setPriority(LocPriority priority);

		void onStringPropertiesModified(const Vector<String>& keys);

		void uploadPendingTranslations(bool force);
		void close();

		std::optional<Colour4f> getRowColour(int idx) const;
    };
}
