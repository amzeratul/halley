#pragma once
#include "localisation_data.h"

namespace Halley {
    class LocalisationRemoteClientUpdater {
    public:
        LocalisationRemoteClientUpdater(Project& project);
        ~LocalisationRemoteClientUpdater();

        void setListeningToClient(bool listening);

        void setLocalStrings(const LocStringSet* local);
        void setRemoteStrings(const LocStringSet* remote);

        void onStringsUpdated();
        void onStringUpdated(const String& key);

    private:
        Project& project;
		std::optional<uint32_t> clientLanguageHandle;

        const LocStringSet* localSet = nullptr;
        const LocStringSet* remoteSet = nullptr;

        struct Client {
            I18NLanguage language;
            HashMap<String, String> strings;
        };
        HashMap<size_t, Client> clients;

        void onClientI18NData(size_t connId, ConfigNode result);
        void sendStringsToClient(size_t connId, std::optional<String> key);
        HashMap<String, String> makeStringDelta(Client& client, const std::optional<String>& key);
        HashMap<String, String> makeOriginalStringDelta(Client& client, const std::optional<String>& filterKey);
        HashMap<String, String> makeTranslatedStringDelta(Client& client, const std::optional<String>& filterKey);
    };
}
