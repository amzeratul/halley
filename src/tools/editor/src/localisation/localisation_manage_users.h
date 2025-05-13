#pragma once

#include "halley/ui/ui_widget.h"
#include "halley/ui/ui_factory.h"
#include "localisation_client.h"

namespace Halley {
	class LocalisationEditorRoot;

	class LocalisationManageUsers : public UIWidget {
    public:
        LocalisationManageUsers(UIFactory& factory, LocalisationClient& client, Project& project, LocalisationEditorRoot& editorRoot);
        ~LocalisationManageUsers() override;

        void onMakeUI() override;

    private:
        UIFactory& factory;
        LocalisationClient& client;
        Project& project;
        LocalisationEditorRoot& editorRoot;

        String currentUserId;

        std::shared_ptr<bool> aliveFlag;
        Vector<LocUserData> curUsers;
        LocProjectData curUserDataWorkingCopy;

        const LocUserData& getUser(const String& userId);

        void addUser();
        void deleteUser();

    	void requestUserList();
        void populateUserList(Vector<LocUserData> users);
        void setCurrentUser(String _userId);

        void setLanguageEnabled(const I18NLanguage& language, bool enabled);
        void updateLanguages();

		void changePassword();
    	void toggleAdmin();
    };
}
