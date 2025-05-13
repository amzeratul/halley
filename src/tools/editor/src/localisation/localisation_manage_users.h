#pragma once

#include "halley/ui/ui_widget.h"
#include "halley/ui/ui_factory.h"
#include "localisation_client.h"

namespace Halley {
	class LocalisationEditorRoot;

	class LocalisationManageUsers : public UIWidget {
    public:
        LocalisationManageUsers(UIFactory& factory, LocalisationClient& client, Project& project, LocalisationEditorRoot& editorRoot);

        void onMakeUI() override;

    private:
        UIFactory& factory;
        LocalisationClient& client;
        Project& project;
        LocalisationEditorRoot& editorRoot;

        String currentUserId;

        AliveFlag aliveFlag;
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

    class LocalisationNewUser : public UIWidget {
    public:
        using Callback = std::function<void(String, String)>;

        LocalisationNewUser(UIFactory& factory, Callback callback);

        void onMakeUI() override;
        void update(Time t, bool moved) override;

    private:
        UIFactory& factory;
        Callback callback;

        String generatePassword();
    };
}
