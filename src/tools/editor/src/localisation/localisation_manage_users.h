#pragma once

#include "halley/ui/ui_widget.h"
#include "halley/ui/ui_factory.h"
#include "localisation_client.h"

namespace Halley {
    class LocalisationManageUsers : public UIWidget {
    public:
        LocalisationManageUsers(UIFactory& factory, LocalisationClient& client);
        ~LocalisationManageUsers();

        void onMakeUI() override;

    private:
        UIFactory& factory;
        LocalisationClient& client;

        String currentUser;

        std::shared_ptr<bool> aliveFlag;

        void addUser();
        void deleteUser();

    	void populateList();
        void setCurrentUser(String user);

        void changePassword();
        void updateLanguages();
    	void toggleAdmin();
    };
}
