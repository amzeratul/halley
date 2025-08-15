#pragma once

namespace Halley {
    class LocalisationRemoteClientUpdater {
    public:
        LocalisationRemoteClientUpdater(Project& project);
        ~LocalisationRemoteClientUpdater();

        void setListeningToClient(bool listening);

    private:
        Project& project;
		std::optional<uint32_t> clientLanguageHandle;

        void onClientI18NData(size_t connId, ConfigNode result);
    };
}
